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
#include <ArduinoHttpClient.h>
#include <mutex>

class Networking {
public:
    explicit Networking(StateManager &stateManager);

    bool isModemConnected();

    bool isMqttConnected();

    bool reportCurrentState(bool forceTimeRefresh);

    void startSleep();

    boolean isSleeping();

    bool wakeUp();

    bool connectToGsm();

    void connectMQTT();

    uint64_t getCurrentTime(bool forceRefresh);

    void resetLastTime();

private:
    StateManager *stateManager;
    TinyGsm modem = TinyGsm(SERIALGSM);
    TinyGsmClient gsmClient1 = TinyGsmClient(modem, 0);
    PubSubClient mqttClient = PubSubClient(gsmClient1);
    TinyGsmClient gsmClient2 = TinyGsmClient(modem, 1);
    HttpClient timeServerClient = HttpClient(gsmClient2, AG_TIMESERVER_HOST, AG_TIMESERVER_PORT);
    std::mutex modem_mutex;
    uint64_t lastTime = 0;
    uint skippedTimeRenewal = 0;
    int timeAdj = AG_TIMESERVER_STARTADJ;
};

#endif //ALARM_GARAGE_NETWORKING_H
