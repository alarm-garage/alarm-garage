#include <Arduino.h>

#include <Tasker.h>

#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883

#define PIN_SLEEP 5

const char apn[] = "internet.t-mobile.cz";
const char user[] = "";
const char pass[] = "";

#define TINY_GSM_MODEM_SIM808
#define SERIALGSM Serial2

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <mutex>


TinyGsm modem(SERIALGSM);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);

boolean started = false, sleeping = false, reconnecting = false;

void start();

std::mutex modem_mutex;
std::mutex mqtt_mutex;

bool isModemConnected() {
    std::lock_guard<std::mutex> lg(mqtt_mutex);
    std::lock_guard<std::mutex> lg2(modem_mutex);
    return modem.isNetworkConnected() && modem.isGprsConnected();
}

bool isMqttConnected() {
    std::lock_guard<std::mutex> lg(mqtt_mutex);
    std::lock_guard<std::mutex> lg2(modem_mutex);
    return mqttClient.connected();
}

bool mqttPublish(const char *topic, const char *payload) {
    std::lock_guard<std::mutex> lg(mqtt_mutex);
    std::lock_guard<std::mutex> lg2(modem_mutex);

    return mqttClient.publish(topic, payload);
}

void sleepEnable(bool enabled) {
    std::lock_guard<std::mutex> lg(modem_mutex);
    std::lock_guard<std::mutex> lg2(mqtt_mutex);

    sleeping = enabled;

    digitalWrite(PIN_SLEEP, enabled ? HIGH : LOW);
    Tasker::sleep(55);
    modem.sleepEnable(enabled);

}

bool connectToGsm() {
    if (reconnecting) return false;

    std::lock_guard<std::mutex> lg(mqtt_mutex);
    std::lock_guard<std::mutex> lg2(modem_mutex);

    reconnecting = true;
    bool modemConnected = false;

    Serial.println(F("Initializing modem..."));
    TinyGsmAutoBaud(SERIALGSM, 9600, 57600);

    if (!modem.init()) {
        Serial.println("Modem init failed!!!");
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
        Serial.print(apn);
        Tasker::yield();
        if (!modem.gprsConnect(apn, user, pass)) {
            Serial.println(" fail");
            Tasker::yield();
            continue;
        }
        Tasker::yield();

        modemConnected = true;
        Serial.println(" OK");
    }

    reconnecting = false;
    return true;
}

void connectMQTT() {
    if (reconnecting) return;

    std::lock_guard<std::mutex> lg(mqtt_mutex);
    std::lock_guard<std::mutex> lg2(modem_mutex);

    reconnecting = true;

    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Tasker::yield();
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        Serial.print("Attempting MQTT connection...");
        mqttClient.disconnect();
        if (mqttClient.connect(clientId.c_str())) {
            Serial.printf(" connected to %s\n", MQTT_HOST);

            if (!started) {
                DefaultTasker.once("Starting", [] {
                    start();
                });
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            Tasker::sleep(5000);
        }
    }

    reconnecting = false;
}

void setup() {
    Serial.begin(9600);

    pinMode(PIN_SLEEP, OUTPUT);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    if (connectToGsm()) { connectMQTT(); }
    else {
        // TODO display
        Serial.println(F("Could not connect to GSM!!!"));
    }

    NetworkTasker.loop("MQTT loop", [] {
        std::lock_guard<std::mutex> lg(mqtt_mutex);
        std::lock_guard<std::mutex> lg2(modem_mutex);

        mqttClient.loop();
    });
}

void loop() {
    // no op - everything is handled by native tasks through Tasker
}

void start() {
    if (!started) {
        Serial.println("Starting the app...");
        started = true;
        sleeping = false;

        // TODO This is weird it has to run on CORE0
        NetworkTasker.loopEvery("Publishing", 1000, [] {
            if (isMqttConnected()) {
                char buffer[50];
                sprintf(buffer, "From start: %lu", millis());

                if (!mqttPublish("jenda-test", buffer)) {
                    Serial.println("Could not send message!");
                }
            } else {
                Serial.println("MQTT disconnected");
            }
        });

        NetworkTasker.loopEvery("Reconnect", 1000, [] {
            if (started && !sleeping && !reconnecting) {
                if (!isModemConnected()) {
                    Serial.println(F("Reconnecting modem..."));
                    Tasker::yield();
                    connectToGsm();
                    return;
                }

                if (!isMqttConnected()) {
                    Serial.println(F("Reconnecting MQTT..."));
                    Tasker::yield();
                    connectMQTT();
                    return;
                }
            }
        });

        Tasker::sleep(2000);
        Serial.println("Sleeep!");
        sleepEnable(true);
        Tasker::sleep(5000);
        Serial.println("Wakeup");
        sleepEnable(false);
    }
}
