#include <Arduino.h>
#include <Tasker.h>
#include <Constants.h>
#include <StateManager.h>
#include <Networking.h>

void toggleArmed();

StateManager stateManager;
Networking networking(stateManager);

void startSleep() {
    Serial.println("Going to sleep...");
    networking.startSleep();

    // TODO sleep of ESP
}

void wakeUpModem() {
    if (!networking.wakeUp()) {
        Serial.println("Could not wake up modem!");
    } else {
        Serial.println("Modem has woken up!");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(AG_PIN_SLEEP, OUTPUT);
    pinMode(AG_PIN_DOOR_SENSOR, INPUT);

    Serial.println("Initializing...");

    if (networking.connectToGsm()) {
        networking.connectMQTT();
    } else {
        // TODO display
        Serial.println(F("Could not connect to GSM!!!"));
        return;
    }

    Serial.println("Starting the app...");
    stateManager.setStarted();

    DefaultTasker.loopEvery("DoorSensor", 500, [] {
        bool open = digitalRead(AG_PIN_DOOR_SENSOR) == 0;

        if (stateManager.handleDoorState(open)) {
            Serial.println("Changed door state:");
            stateManager.printCurrentState();

            // TODO do something real - this just puts modem to sleep when doors are closed and wakes him once doors are open
            if (!open) {
                startSleep();
            } else {
                wakeUpModem();
            }
        }
    });

    Serial.println("Current state:");
    stateManager.printCurrentState();

    // TODO This is weird it has to run on CORE0
    NetworkTasker.loopEvery("Publishing", 1000, [] {
        if (!networking.isSleeping()) {
            if (networking.isMqttConnected()) {
                char buffer[50];
                sprintf(buffer, "From start: %lu", millis());

                if (!networking.mqttPublish("jenda-test", buffer)) {
                    Serial.println("Could not send message!");
                }
            } else {
                Serial.println("MQTT disconnected");
            }
        }
    });
}

void loop() {
    // no op - everything is handled by native tasks through Tasker
}

void toggleArmed() {
    stateManager.toggleArmed();
    Serial.println("Changed armed state:");
    stateManager.printCurrentState();
}
