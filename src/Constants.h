#ifndef ALARM_GARAGE_CONSTANTS_H
#define ALARM_GARAGE_CONSTANTS_H

#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_TOPIC "jenda-test"

#define AG_PIN_SLEEP 15
#define AG_PIN_DOOR_SENSOR 33

#define AG_WAKEUP_RADIO_PIN GPIO_NUM_32
#define AG_WAKEUP_DOORS_BITMASK 0x200000000

#define AG_PIN_RF_CE 4
#define AG_PIN_RF_CSN 5

#define APN_HOST "internet.t-mobile.cz"
#define APN_USER ""
#define APN_PASSWORD ""

#endif //ALARM_GARAGE_CONSTANTS_H
