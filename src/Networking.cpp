#include <Networking.h>
#include <proto/pb_encode.h>
#include <proto/alarm.pb.h>

bool Networking::isModemConnected() {
    std::lock_guard<std::mutex> lg(modem_mutex);
    return modem.isNetworkConnected() && modem.isGprsConnected();
}

bool Networking::isMqttConnected() {
    std::lock_guard<std::mutex> lg(modem_mutex);
    return mqttClient.connected();
}

void Networking::startSleep() {
    std::lock_guard<std::mutex> lg(modem_mutex);
    Serial.println("Putting modem to sleep");

    digitalWrite(AG_PIN_SLEEP, HIGH);
    Tasker::sleep(55);
    modem.sleepEnable(true);
    stateManager->setModemSleeping(true);
}

boolean Networking::isSleeping() {
    return stateManager->isModemSleeping();
}

bool Networking::wakeUp() {
    { // this exists to release mutex ASAP
        std::lock_guard<std::mutex> lg(modem_mutex);
        Serial.println("Waking up modem");

        digitalWrite(AG_PIN_SLEEP, LOW);
        Tasker::sleep(55);
        modem.sleepEnable(false);
    }

    Tasker::sleep(500);

    stateManager->setModemSleeping(false);

    return isModemConnected() && isMqttConnected();
}

bool Networking::connectToGsm() {
    std::lock_guard<std::mutex> lg(modem_mutex);

    if (stateManager->isReconnecting()) return false;
    stateManager->toggleReconnecting();

    stateManager->setModemSleeping(false);

    bool modemConnected = false;

    Serial.println(F("Initializing modem..."));
    TinyGsmAutoBaud(SERIALGSM, 9600, 57600);

    if (!modem.init()) {
        Serial.println("Modem init failed!!!");
        // TODO handle
        return false;
    }

    String modemInfo = modem.getModemInfo();
    Serial.print(F("Modem: "));
    Serial.println(modemInfo);

    Tasker::yield();

    while (!modemConnected) {
        Serial.print(F("Waiting for network..."));
        if (!modem.waitForNetwork()) {
            Serial.println(" fail");
            Tasker::yield();
            continue;
        }
        Serial.println(" OK");
        Tasker::yield();

        Serial.print(F("Connecting to "));
        Serial.print(APN_HOST);
        Tasker::yield();
        if (!modem.gprsConnect(APN_HOST, APN_USER, APN_PASSWORD)) {
            Serial.println(" fail");
            Tasker::yield();
            continue;
        }
        Tasker::yield();

        modemConnected = true;
        Serial.println(" OK");
    }

    stateManager->toggleReconnecting();
    return true;
}

void Networking::connectMQTT() {
    std::lock_guard<std::mutex> lg(modem_mutex);

    if (stateManager->isReconnecting()) return;
    stateManager->toggleReconnecting();

    stateManager->setModemSleeping(false);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Tasker::yield();
        // Create a random client ID
        String clientId = "ESPClient-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        Serial.print("Attempting MQTT connection...");
        mqttClient.disconnect();
        if (mqttClient.connect(clientId.c_str())) {
            Serial.printf(" connected to %s\n", MQTT_HOST);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            Tasker::sleep(5000);
        }
    }

    stateManager->toggleReconnecting();
}

Networking::Networking(StateManager &stateManager) {
    this->stateManager = &stateManager;

    NetworkTasker.loopEvery("Reconnect", 1000, [this] {
        if (this->stateManager->isStarted() && !this->stateManager->isModemSleeping() && !this->stateManager->isReconnecting()) {
            if (!isModemConnected()) {
                Serial.println(F("Reconnecting modem..."));
                Tasker::yield();
                this->connectToGsm();
                return;
            }

            if (!this->isMqttConnected()) {
                Serial.println(F("Reconnecting MQTT..."));
                Tasker::yield();
                this->connectMQTT();
                return;
            }
        }
    });
}

bool Networking::reportCurrentState() {
    uint8_t buffer[128];
    size_t message_length;
    bool status;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    _protocol_State message = stateManager->snapshot();

    Serial.printf(
            "Reporting status: Started = %d Sleeping = %d Reconnecting = %d Armed = %d DoorsOpen = %d\n",
            message.started, message.modem_sleeping, message.reconnecting, message.armed, message.doors_open
    );

    status = pb_encode(&stream, protocol_State_fields, &message);
    message_length = stream.bytes_written;

    if (!status) {
        Serial.println("Could not encode status message!");
        return false;
    }

    std::lock_guard<std::mutex> lg(modem_mutex);
    return mqttClient.publish(MQTT_TOPIC, buffer, message_length, true);
}
