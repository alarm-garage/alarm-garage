#ifndef ALARM_GARAGE_STATEMANAGER_H
#define ALARM_GARAGE_STATEMANAGER_H

#include <Arduino.h>

class StateManager {
public:
    StateManager();

    void toggleArmed();

    void printCurrentState() const;

    void setStarted();

    void setModemSleeping(bool sleeping);

    void toggleReconnecting();

    bool isReconnecting() const;

    bool isStarted() const;

    bool isModemSleeping() const;

    bool handleDoorState(bool open);

private:
    boolean started;
    boolean modemSleeping;
    boolean reconnecting;
    boolean armed;
    boolean doorsOpen;
};

#endif //ALARM_GARAGE_STATEMANAGER_H
