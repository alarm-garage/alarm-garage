#ifndef ALARM_GARAGE_RADIO_H
#define ALARM_GARAGE_RADIO_H

#include <Constants.h>
#include <RF24.h>

const byte addressReceive[6] = "1tran";
const byte addressSend[6] = "2tran";

class Radio {
public:
    bool init();
    uint8_t radioReceive(byte *buff);
    bool radioSend(byte *buff, uint8_t len);
private:
    RF24 radio = RF24(AG_PIN_RF_CE, AG_PIN_RF_CSN);
};

#endif //ALARM_GARAGE_RADIO_H
