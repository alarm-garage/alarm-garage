#include "Radio.h"
#include "printf.h"

bool Radio::init() {
    Serial.println("Initializing radio...");

    if (!radio.begin()) {
        return false;
    }

    printf_begin();

    radio.setAutoAck(true);
    radio.enableDynamicPayloads();
    radio.setPayloadSize(AG_RADIO_PAYLOAD_SIZE);
    radio.setDataRate(RF24_250KBPS);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.setChannel(109);
    radio.openReadingPipe(1, addressReceive);
    radio.openWritingPipe(addressSend);
    radio.startListening();
    radio.printDetails();

    return true;
}

bool Radio::radioReceive(byte buff[AG_RADIO_PAYLOAD_SIZE]) {
    if (radio.available()) {
        radio.read(buff, AG_RADIO_PAYLOAD_SIZE);
        return true;
    }

    return false;
}

bool Radio::radioSend(byte buff[AG_RADIO_PAYLOAD_SIZE]) {
    radio.stopListening();
    bool r = radio.write(buff, AG_RADIO_PAYLOAD_SIZE);
    radio.startListening();
    return r;
}
