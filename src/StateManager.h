#ifndef ALARM_GARAGE_STATEMANAGER_H
#define ALARM_GARAGE_STATEMANAGER_H

#include <Arduino.h>
#include <proto/alarm.pb.h>

class StateManager {
public:
    void toggleArmed();

    void printCurrentState() const;

    void setStarted(bool doorsOpen);

    void setModemSleeping(bool sleeping);

    void toggleReconnecting();

    bool isReconnecting() const;

    bool isStarted() const;

    bool isModemSleeping() const;

    bool handleDoorState(bool open);

    _protocol_State snapshot();

private:
    _protocol_State inner = protocol_State_init_zero;
};

#endif //ALARM_GARAGE_STATEMANAGER_H
