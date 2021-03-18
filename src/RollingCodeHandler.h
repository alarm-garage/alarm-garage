#ifndef ALARM_GARAGE_ROLLINGCODEHANDLER_H
#define ALARM_GARAGE_ROLLINGCODEHANDLER_H

#include <Arduino.h>
#include <Constants.h>
#include <Crypto.h>
#include <Ascon128.h>
#include "utility/ProgMemUtil.h"

#include <SimpleMap.h>
#include <ArduinoJson.h>

#include "Radio.h"
#include "StateManager.h"

#include "SPIFFS.h"
#include <mutex>

#include <proto/pb_decode.h>
#include <proto/alarm.pb.h>
#include <proto/pb_encode.h>

typedef struct ClientState {
    byte key[AG_SIGNAL_KEY_SIZE];
    byte authData[AG_SIGNAL_AUTH_SIZE];
    uint32_t lastCode;
} ClientState;

class RollingCodeHandler {
public:
    explicit RollingCodeHandler(StateManager &stateManager, Radio &radio);

    bool handleRadioSignal();

    bool init();

private:
    bool decryptSignal(byte *buff,
                       const byte key[AG_SIGNAL_KEY_SIZE],
                       const byte authData[AG_SIGNAL_AUTH_SIZE],
                       const _protocol_RemoteSignal *signal);

    bool encryptSignalResponsePayload(_protocol_RemoteSignalResponse *output,
                                      const byte *key,
                                      const byte *authData,
                                      _protocol_RemoteSignalResponsePayload payload);

    bool persistStates();

    bool loadStates();

    Ascon128 cipher;
    Radio *radio;
    StateManager *stateManager;
    std::mutex mutex;
    SimpleMap<String, ClientState> *clients;
    byte iv[AG_SIGNAL_IV_SIZE] = AG_SIGNAL_IV;
};


#endif //ALARM_GARAGE_ROLLINGCODEHANDLER_H
