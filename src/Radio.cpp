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

uint8_t Radio::radioReceive(byte *buff) {
    if (radio.available()) {
        const uint8_t size = radio.getDynamicPayloadSize();
        radio.read(buff, size);
        return size;
    }

    return -1;
}

bool Radio::radioSend(byte *buff, uint8_t len) {
    radio.stopListening();
    bool r = radio.write(buff, len);
    radio.startListening();
    return r;
}
