#include <Arduino.h>
#include <Tasker.h>
#include <Constants.h>
#include <StateManager.h>
#include <RollingCodeManager.h>
#include <Networking.h>
#include <Radio.h>

void wakeUpAndSendReport(bool forceTimeRefresh);

bool areDoorsOpen() {
    return digitalRead(AG_PIN_DOOR_SENSOR) == HIGH;
}

RollingCodeManager codeManager;
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

byte rawReceivedData[AG_RADIO_PAYLOAD_SIZE];

void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(AG_PIN_SLEEP, OUTPUT);
    pinMode(AG_PIN_DOOR_SENSOR, INPUT_PULLUP);

    Serial.println(F("Initializing..."));

    if (!SPIFFS.begin()) {
        Serial.println("Could not init FS");
        return;
    }

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

    if (!codeManager.init()) {
        Serial.println("Could not init code manager");
        return;
    }

    stateManager.setStarted(areDoorsOpen());

    // plan wakeups
    esp_sleep_enable_ext0_wakeup(AG_WAKEUP_RADIO_PIN, LOW);
    esp_sleep_enable_ext1_wakeup(AG_WAKEUP_DOORS_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_enable_timer_wakeup(AG_WAKEUP_PERIOD_SECS * 1000000);

    // report current state right during the startup
    if (!networking.reportCurrentState(true)) {
        Serial.println("Could not report state!");
        Serial.printf("Modem connected: %d\n", networking.isMqttConnected());
    }

    DefaultTasker.loop("SleepingLoop", [] {
        startSleep();

        esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

        switch (wakeupReason) {
            case AG_WAKEUP_REASON_RADIO:
                Serial.println("Radio caused wakeup!");
                networking.resetLastTime();
                break;
            case AG_WAKEUP_REASON_DOORS: {
                Serial.println("Doors caused wakeup!");
                networking.resetLastTime();
                esp_sleep_disable_wakeup_source(AG_WAKEUP_REASON_DOORS);
            }
                break;
            case AG_WAKEUP_REASON_TIMER: {
                Serial.println("Timer caused wakeup!");
            }
                break;
            default:
                Serial.println("Different source of wakeup!");
        }

        if (radio.radioReceive(rawReceivedData)) {
            if (codeManager.validate(rawReceivedData)) {
                Serial.println("Signal validated!");
                stateManager.toggleArmed();
            }
        }

        bool doorsOpen = areDoorsOpen();
        Serial.printf("Doors open: %d\n", doorsOpen);
        if (!doorsOpen) {
            Serial.println("Planning interrupt for doors again");
            esp_sleep_enable_ext1_wakeup(AG_WAKEUP_DOORS_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
        }

        if (stateManager.handleDoorState(doorsOpen) || wakeupReason == AG_WAKEUP_REASON_TIMER) {
            // force time refresh if the wakeup source was not timer
            wakeUpAndSendReport(wakeupReason != AG_WAKEUP_REASON_TIMER);
        }
    });

    Serial.println(F("Startup finished!"));
}


void loop() {
    // no op - everything is handled by native tasks through Tasker
}

void wakeUpAndSendReport(bool forceTimeRefresh) {
    if (networking.wakeUp()) {
        Serial.println("Modem has woken up!");
        if (!networking.reportCurrentState(forceTimeRefresh)) {
            Serial.println("Could not report state!");
            Serial.printf("Modem connected: %d\n", networking.isMqttConnected());
        }
    } else {
        Serial.println("Could not wake up modem!");
    }
}
