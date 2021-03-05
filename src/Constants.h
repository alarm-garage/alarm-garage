#ifndef ALARM_GARAGE_CONSTANTS_H
#define ALARM_GARAGE_CONSTANTS_H

#define AG_MQTT_HOST "test.mosquitto.org"
#define AG_MQTT_PORT 1883
#define AG_MQTT_TOPIC "jenda-test"

#define AG_WAKEUP_PERIOD_SECS 120
#define AG_WAKEUP_RADIO_PIN GPIO_NUM_32
#define AG_WAKEUP_DOORS_BITMASK 0x200000000
#define AG_WAKEUP_REASON_DOORS ESP_SLEEP_WAKEUP_EXT1
#define AG_WAKEUP_REASON_RADIO ESP_SLEEP_WAKEUP_EXT0
#define AG_WAKEUP_REASON_TIMER ESP_SLEEP_WAKEUP_TIMER

#define AG_PIN_SLEEP 15
#define AG_PIN_DOOR_SENSOR 33

#define AG_PIN_RF_CE 4
#define AG_PIN_RF_CSN 5

#define AG_APN_HOST "internet.t-mobile.cz"
#define AG_APN_USER ""
#define AG_APN_PASSWORD ""

#define AG_TIMESERVER_HOST "showcase.api.linx.twenty57.net"
#define AG_TIMESERVER_PORT 80
#define AG_TIMESERVER_PATH "/UnixTime/tounix?date=now"

#define AG_TIMESERVER_SKIPS 10
#define AG_TIMESERVER_STARTADJ 1500 // a magic, empiric value!

#define AG_RADIO_PAYLOAD_SIZE 32

#define AG_SIGNAL_AUTH_SIZE 4
#define AG_SIGNAL_DATA_SIZE 14
#define AG_SIGNAL_AUTH_TAG_SIZE 8
#define AG_SIGNAL_KEY_SIZE 16
#define AG_SIGNAL_IV_SIZE 16
#define AG_SIGNAL_IV {0x91, 0x5f, 0xf8, 0xff, 0xca, 0xd8, 0xae, 0x1d, 0xf4, 0x45, 0xeb, 0x03, 0xe2, 0x18, 0xfd, 0x25}

#endif //ALARM_GARAGE_CONSTANTS_H
