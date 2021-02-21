#include <StateManager.h>

void StateManager::toggleArmed() {
    inner.armed = !inner.armed;
    Serial.println("Changed armed state:");
    printCurrentState();
}

void StateManager::printCurrentState() const {
    Serial.printf(
            "Started = %d Sleeping = %d Reconnecting = %d Armed = %d DoorsOpen = %d\n",
            inner.started, inner.modem_sleeping, inner.reconnecting, inner.armed, inner.doors_open
    );
}

void StateManager::setStarted(bool doors_open) {
    inner.started = true;
    this->inner.doors_open = doors_open;
}

void StateManager::setModemSleeping(bool sleeping) {
    inner.modem_sleeping = sleeping;
}

void StateManager::toggleReconnecting() {
    inner.reconnecting = !inner.reconnecting;
}

bool StateManager::isReconnecting() const {
    return inner.reconnecting;
}

bool StateManager::isStarted() const {
    return inner.started;
}

bool StateManager::isModemSleeping() const {
    return inner.modem_sleeping;
}

bool StateManager::handleDoorState(bool open) {
    if (open != inner.doors_open) {
        inner.doors_open = open;
        return true;
    }

    return false;
}

_protocol_State StateManager::snapshot() {
    return inner;
}
