#ifndef ALARM_GARAGE_ROLLINGCODEMANAGER_H
#define ALARM_GARAGE_ROLLINGCODEMANAGER_H

#include <Arduino.h>
#include <Constants.h>
#include <Crypto.h>
#include <Ascon128.h>
#include "utility/ProgMemUtil.h"

#include <SimpleMap.h>
#include <ArduinoJson.h>

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

class RollingCodeManager {
public:
    explicit RollingCodeManager();

    bool validate(byte rawSignal[AG_RADIO_PAYLOAD_SIZE]);

    bool init();

private:
    bool decrypt_signal(byte *decrypted,
                        const byte key[AG_SIGNAL_KEY_SIZE],
                        const byte authData[AG_SIGNAL_AUTH_SIZE],
                        const _protocol_RemoteSignal *signal);

    bool persistStates();

    bool loadStates();

    Ascon128 cipher;
    std::mutex mutex;
    SimpleMap<String, ClientState> *clients;
    byte iv[AG_SIGNAL_IV_SIZE] = AG_SIGNAL_IV;
};


#endif //ALARM_GARAGE_ROLLINGCODEMANAGER_H
