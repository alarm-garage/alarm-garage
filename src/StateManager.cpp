#include <StateManager.h>

StateManager::StateManager(){
    started = false;
    modemSleeping = false;
    reconnecting = false;
    armed = false;
    doorsOpen = false; // temporary
}

void StateManager::toggleArmed() {
    armed = !armed;
    Serial.println("Changed armed state:");
    printCurrentState();
}

void StateManager::printCurrentState() const {
    Serial.printf(
            "Started = %d Sleeping = %d Reconnecting = %d Armed = %d DoorsOpen = %d\n",
            started, modemSleeping, reconnecting, armed, doorsOpen
    );
}

void StateManager::setStarted(bool doorsOpen) {
    started = true;
    this->doorsOpen = doorsOpen;
}

void StateManager::setModemSleeping(bool sleeping) {
    modemSleeping = sleeping;
}

void StateManager::toggleReconnecting() {
    reconnecting = !reconnecting;
}

bool StateManager::isReconnecting() const {
    return reconnecting;
}

bool StateManager::isStarted() const {
    return started;
}

bool StateManager::isModemSleeping() const {
    return modemSleeping;
}

bool StateManager::handleDoorState(bool open) {
    if (open != doorsOpen) {
        doorsOpen = open;
        return true;
    }

    return false;
}
