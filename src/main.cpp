#include <Arduino.h>

#include <Tasker.h>

#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883

const char apn[] = "internet.t-mobile.cz";
const char user[] = "";
const char pass[] = "";

#define TINY_GSM_MODEM_SIM808
#define SERIALGSM Serial2

#include <TinyGsmClient.h>
#include <PubSubClient.h>


TinyGsm modem(SERIALGSM);
TinyGsmClient gsmClient(modem);
PubSubClient mqttClient(gsmClient);


bool connectToGsm() {
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

    while (!modemConnected) {
        Serial.print(F("Waiting for network..."));
        if (!modem.waitForNetwork()) {
            Serial.println(" fail");
            delay(1000);
            return false;
        }
        Serial.println(" OK");

        Serial.print(F("Connecting to "));
        Serial.print(apn);
        if (!modem.gprsConnect(apn, user, pass)) {
            Serial.println(" fail");
            delay(1000);
            return false;
        }

        modemConnected = true;
        Serial.println(" OK");
    }

    return true;
}

boolean started = false;

void start();

void reconnect() {
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str())) {
            Serial.printf(" connected to %s\n", MQTT_HOST);
            start();
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(9600);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    NetworkTasker.once([] {
        if (connectToGsm()) { reconnect(); }
        else {
            // TODO display
            Serial.println("Could not connect to GSM!!!");
        }
    });

    NetworkTasker.loop([] {
        if (started) {
            if (!mqttClient.connected()) {
                reconnect();
                return;
            }

            mqttClient.loop();
        }
    });
}

void loop() {
    // no op - everything is handled by native tasks through Tasker
}

void start() {
    if (!started) {
        Serial.println("Starting the app");

        DefaultTasker.loopEvery(1000, [] {
            if (mqttClient.connected()) {
                char buffer[50];
                sprintf(buffer, "From start: %lu", millis());

                if (!mqttClient.publish("jenda-test", buffer, true)) {
                    Serial.println("Could not send message!");
                }
            }
        });
    }
}
