#ifndef ALARM_GARAGE_RADIO_H
#define ALARM_GARAGE_RADIO_H

#include <Constants.h>
#include <RF24.h>

const byte addressReceive[6] = "1tran";
const byte addressSend[6] = "2tran";

class Radio {
public:
    bool init();
    bool radioReceive(byte buff[AG_RADIO_PAYLOAD_SIZE]);
    bool radioSend(byte buff[AG_RADIO_PAYLOAD_SIZE]);
private:
    RF24 radio = RF24(AG_PIN_RF_CE, AG_PIN_RF_CSN);
};

#endif //ALARM_GARAGE_RADIO_H
