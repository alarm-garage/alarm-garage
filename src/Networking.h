#ifndef ALARM_GARAGE_NETWORKING_H
#define ALARM_GARAGE_NETWORKING_H

#define TINY_GSM_MODEM_SIM808
#define SERIALGSM Serial2

#include <Arduino.h>
#include <Constants.h>
#include <Tasker.h>
#include <StateManager.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <mutex>

class Networking {
public:
    explicit Networking(const StateManager &stateManager);

    bool isModemConnected();

    bool isMqttConnected();

    bool mqttPublish(const char *topic, const char *payload);

    void startSleep();

    boolean isSleeping();

    bool wakeUp();

    bool connectToGsm();

    void connectMQTT();

private:
    StateManager stateManager;
    TinyGsm modem = TinyGsm(SERIALGSM);
    TinyGsmClient gsmClient = TinyGsmClient(modem);
    PubSubClient mqttClient = PubSubClient(gsmClient);
    std::mutex modem_mutex;
};

#endif //ALARM_GARAGE_NETWORKING_H
