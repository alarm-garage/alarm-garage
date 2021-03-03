#include <Networking.h>
#include <proto/pb_encode.h>
#include <proto/alarm.pb.h>
#include <sstream>

Networking::Networking(StateManager &stateManager) {
    this->stateManager = &stateManager;
    timeServerClient.connectionKeepAlive();
}

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

    Serial.println("Waiting for modem to wake up...");
    Tasker::sleep(150);

    stateManager->setModemSleeping(false);

    while (!(isModemConnected() && isMqttConnected())) {
        if (!isModemConnected()) {
            Serial.println(F("Reconnecting modem..."));
            Tasker::yield();
            if (!this->connectToGsm()) {
                return false;
            }
        }

        if (!this->isMqttConnected()) {
            Serial.println(F("Reconnecting MQTT..."));
            Tasker::yield();
            this->connectMQTT();
        }
    }

    return isModemConnected() && isMqttConnected();
}

bool Networking::connectToGsm() {
    std::lock_guard<std::mutex> lg(modem_mutex);

    if (stateManager->isReconnecting()) return false;
    stateManager->toggleReconnecting();

    // send wakeup signal.. just to be sure ;-)
    digitalWrite(AG_PIN_SLEEP, LOW);
    Tasker::sleep(55);
    modem.sleepEnable(false);

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

        Serial.printf("Connecting to %s", AG_APN_HOST);

        if (!modem.gprsConnect(AG_APN_HOST, AG_APN_USER, AG_APN_PASSWORD)) {
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

    mqttClient.setServer(AG_MQTT_HOST, AG_MQTT_PORT);
    mqttClient.setKeepAlive(300);

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
            Serial.printf(" connected to %s\n", AG_MQTT_HOST);
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

bool Networking::reportCurrentState(bool forceTimeRefresh) {
    uint8_t buffer[128];
    size_t message_length;
    bool status;

    if (!(isModemConnected() && isMqttConnected())) {
        Serial.println("Not connected - could not report state");
        return false;
    }

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    _protocol_Report report = protocol_Report_init_zero;
    _protocol_State state = stateManager->snapshot();

    Serial.printf(
            "Reporting status: Started = %d Sleeping = %d Reconnecting = %d Armed = %d DoorsOpen = %d\n",
            state.started, state.modem_sleeping, state.reconnecting, state.armed, state.doors_open
    );

    report.state = state;

    uint64_t currentTime = getCurrentTime(forceTimeRefresh);
    if (currentTime > 0) {
        report.timestamp = currentTime;
        report.has_timestamp = true;
    }

    status = pb_encode(&stream, protocol_Report_fields, &report);
    message_length = stream.bytes_written;

    if (!status) {
        Serial.println("Could not encode status report!");
        return false;
    }

    std::lock_guard<std::mutex> lg(modem_mutex);
    return mqttClient.publish(AG_MQTT_TOPIC, buffer, message_length, true);
}

uint64_t Networking::getCurrentTime(bool forceRefresh) {
    uint64_t guessedTime = lastTime + AG_WAKEUP_PERIOD_SECS * 1000L + timeAdj;

    if (!forceRefresh && skippedTimeRenewal < AG_TIMESERVER_SKIPS && lastTime > 0) {
        lastTime = guessedTime;
        skippedTimeRenewal++;
        Serial.printf("Guessed current time: %llu\n", lastTime);
        return lastTime;
    }

    if (!isModemConnected()) {
        Serial.println("Not connected - could not get current time");
        return -1;
    }

    ulong start = millis();

    if (!timeServerClient.connected()) {
        Serial.println("Time client not connected :-(");
    }

    std::lock_guard<std::mutex> lg(modem_mutex);

    if (timeServerClient.get(AG_TIMESERVER_PATH) != 0) {
        Serial.println("Could not get current time");
        return -1;
    }

    int statusCode = timeServerClient.responseStatusCode();
    const String &body = timeServerClient.responseBody();

    if (statusCode != 200) {
        Serial.printf("Could not get current time, status HTTP %d, body: %s\n", statusCode, body.c_str());
        return -1;
    }

    if (!timeServerClient.connected()) {
        Serial.println("AFTER Time client not connected :-(");
    }

    uint64_t currentTime;
    std::istringstream iss(body.c_str());
    iss >> currentTime;

    currentTime *= 1000;
    guessedTime += (millis() - start); // duration of the update above

    if (!forceRefresh && lastTime > 0) {
        uint64_t diff = currentTime - guessedTime;
        timeAdj += ((int) diff) / (AG_TIMESERVER_SKIPS + 1);
        Serial.printf("Guessed it's %llu but it's %llu; new timeAdj: %d\n", guessedTime, currentTime, timeAdj);
    }

    lastTime = currentTime;
    skippedTimeRenewal = 0;

    Serial.printf("Current time: %llu\n", currentTime);

    return lastTime;
}

void Networking::resetLastTime() {
    Serial.println("Resetting last time");
    lastTime = 0;
}
