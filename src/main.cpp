#include <Arduino.h>

#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <Tasker.h>

#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void connectToWifi() {
    const char *ssid = "Hrosinec";
    const char *password = "hrochLupinek";
    Serial.printf("Using SSID %s and password %s\n", ssid, password);

    WiFi.softAPdisconnect(true); // for case it was configured as access point in last run...
    Serial.print(F("Connecting to "));
    Serial.println(ssid);

    WiFi.begin(ssid, password);
}

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            connectToMqtt();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection");
            xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            xTimerStart(wifiReconnectTimer, 0);
            break;
    }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");

    if (WiFi.isConnected()) {
        xTimerStart(mqttReconnectTimer, 0);
    }
}

boolean started = false;

void start(bool sessionPresent) {
    Serial.println("Connected to MQTT");

    if (!started) {
        Serial.println("Starting the app");

        DefaultTasker.loopEvery(1000, [] {
            if (mqttClient.connected()) {
                char buffer[50];
                sprintf(buffer, "From start: %lu", millis());

                if (mqttClient.publish("jenda-test", 0, true, buffer) == 0) {
                    Serial.println("Could not send message!");
                }
            }
        });
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();

    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *) 0,
                                      reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *) 0,
                                      reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(WiFiEvent);

    mqttClient.onConnect(start);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    NetworkTasker.once([] {
        connectToWifi();
    });
}

void loop() {
    // no op - everything is handled by native tasks through Tasker
}
