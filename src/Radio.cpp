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
    radio.openReadingPipe(1, address);
    radio.startListening();
    radio.printDetails();

    return true;
}

bool Radio::radioReceive(char *buff) {
    byte clientId = 0;
    if (radio.available(&clientId)) {
        radio.read(buff, 32);
        return true;
    }

    return false;
}
