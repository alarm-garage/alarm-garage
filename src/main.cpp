#include <Arduino.h>
#include <Tasker.h>
#include <Constants.h>
#include <StateManager.h>
#include <Networking.h>
#include <Radio.h>

void toggleArmed();

void wakeUpAndSendReport();

bool areDoorsOpen() {
    return digitalRead(AG_PIN_DOOR_SENSOR) == HIGH;
}

StateManager stateManager;
Networking networking(stateManager);
Radio radio;

void startSleep() {
    Serial.println("Going to sleep...");
    if (!networking.isSleeping()) networking.startSleep();

    Tasker::sleep(100);

    esp_light_sleep_start();
    Tasker::sleep(100);

    Serial.println("-----------------------------");
    Serial.println("Woken up!");
}

char rawReceivedData[33];

void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(AG_PIN_SLEEP, OUTPUT);
    pinMode(AG_PIN_DOOR_SENSOR, INPUT_PULLUP);

    Serial.println(F("Initializing..."));

    if (!radio.init()) {
        // TODO display
        Serial.println(F("Could not initialize radio!!!"));
        return;
    }

    if (networking.connectToGsm()) {
        networking.connectMQTT();
    } else {
        // TODO display
        Serial.println(F("Could not connect to GSM!!!"));
        return;
    }

    Serial.println(F("Starting the app..."));

    bool doorsOpen = areDoorsOpen();
    Serial.printf("Doors open: %d\n", doorsOpen);
    stateManager.setStarted(doorsOpen);

    esp_sleep_enable_ext0_wakeup(AG_WAKEUP_RADIO_PIN, LOW);
    esp_sleep_enable_ext1_wakeup(AG_WAKEUP_DOORS_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_timer_wakeup(15 * 1000000);

    DefaultTasker.loop("SleepingLoop", [] {
        startSleep();

        switch (esp_sleep_get_wakeup_cause()) {
            case ESP_SLEEP_WAKEUP_EXT0:
                Serial.println("Radio caused wakeup!");
                break;
            case ESP_SLEEP_WAKEUP_EXT1: {
                Serial.println("Doors caused wakeup!");
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
            }
                break;
            case ESP_SLEEP_WAKEUP_TIMER: {
                Serial.println("Timer caused wakeup!");
                wakeUpAndSendReport();
            }
                break;
            default:
                Serial.println("Different source of wakeup!");
        }

        if (radio.radioReceive(rawReceivedData)) {
            Serial.printf("Radio data receive: '%s'\n", rawReceivedData);
            toggleArmed();
        }

        bool doorsOpen = areDoorsOpen();
        Serial.printf("Doors open: %d\n", doorsOpen);
        if (!doorsOpen) {
            Serial.println("Planning interrupt for doors again");
            esp_sleep_enable_ext1_wakeup(AG_WAKEUP_DOORS_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
        }

        if (stateManager.handleDoorState(doorsOpen)) {
            Serial.println(F("Changed door state:"));
            stateManager.printCurrentState();
            wakeUpAndSendReport();
        }
    });

    Serial.println(F("Current state:"));
    stateManager.printCurrentState();
    if (!networking.reportCurrentState()) {
        Serial.println("Could not report state!");
        Serial.printf("Modem connected: %d\n", networking.isMqttConnected());
    }
}


void loop() {
    // no op - everything is handled by native tasks through Tasker
}

void toggleArmed() {
    stateManager.toggleArmed();
    Serial.println("Changed armed state:");
    stateManager.printCurrentState();
}

void wakeUpAndSendReport() {
    if (networking.wakeUp()) {
        Serial.println("Modem has woken up!");
        if (!networking.reportCurrentState()) {
            Serial.println("Could not report state!");
        }
    } else {
        Serial.println("Could not wake up modem!");
    }
}
