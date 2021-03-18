# Main Module

TL;DR: ESP32, WARCAR SIM808 module, nRF24L01, a door switch - _glued_ together in a 3D printer-made box hanging on a wall in garage.

## Technical challenges

### The board

The device is based on ESP32. That may be quite a overkill for such device butI have a really good experience with it and, as Arduino would
not be enough (man, it has no memory!!), from ESP8266 and ESP32 I chose the stronger one.

### Power consumption

There is no electricity in the garage so we have to use a battery to power the device. There are few possibilities where using just an
ordinary power bank is the easiest one (not necessarily the best...) because it gives us 5V USB output and we can easily switch it from
another one and take this one to home for charging.

Let's face it - the device can rest for most of the time and do something only when needed. ESP has a great feature for this -
[several sleep modes](https://lastminuteengineers.com/esp32-sleep-modes-power-consumption/). It would be lovely if the device could
deep-sleep for the whole time and be woken up just by:

1. Timer, each X seconds (120 seems like a good value to me)
1. Door switch (when someone opens the door)
1. Radio ([remote switch](/module-remote) of "armed" state)

Unfortunately, the radio needs some assistance of the chip to work so it can't wake the ESP when it's deep-sleeping :-( That kind of ruins
the idea of low-power device. Fortunately, there's still a light sleep mode - and it works just fine with it!

The problem is, that light-sleeping ESP with activated nRF24L01 still eats cca 46 mA (on 5V) so we're on sth like 13 days of life with a
20000 mAh power bank (the 20000m Ah is for name voltage of its batteries which is usually 3.7V, that means 74 Wh => 14800 mAh/5V which means
320 hours of life, if we don't take any conversion losses into account, which we should). And I didn't even mention the SIM 808 module which
has its own needs of ??? mA).

TODO

### Connectivity

As I need to know whether the device is alive or not, it has to have connectivity to the internet. A good way how to send reports seems to
be the MQTT protocol, which is a queuing system simplified for usage in low-power and low-memory devices. Connecting to the server is quite
expensive so the client has to support keepalive and hold the connection even when the device is sleeping.  
When alert conditions happen, it has to immediately send the report as well as SMS to preconfigured phone number.

### Time

I need a timestamp to be present in every status message - so it's clear when the message was created/sent.  
The solutions are basically three:

1. Use RTC module attached to the board.
1. Download current timestamp whenever needed [from the Internet](http://showcase.api.linx.twenty57.net/UnixTime/tounix?date=now).
1. Download current timestamp once and then keep its approximate value by self-increasing the value.

Because the device sleeps for most of the time (see [#power consumption](#power-consumption)), it just can't increase the timestamp every
second. Using RTC module means yet another HW and is maybe unnecessary precise. Calling HTTP endpoint (fortunately they really support HTTP
which is totally OK here and don't waste resources on my side) is very expensive as the only client I could use was just unable to use
keep-alive.

As described in [#power consumption](#power-consumption), the device wakes in certain interval. We know its length and therefore we're able
to calculate approx. time increase - so we just add e.g. 120 secs every time? Almost. Handling of the report sending takes some time too and
we need to include it. So, long story short - there is a `timeAdj` variable in [`Networking`][Networking.cpp] which holds how much time (on
top of those 120 seconds) one cycle took. With this, we download the current time only once per N iterations and based on difference between
_guessed_ and real time, we adjust the `timeAdj` for next N cycles.  
Of course, this is real only when the device is woken up by the timer - if radio ([remote](/module-remote)) or doors switch was the wakeup
cause, we have no idea how much to add to the last timestamp so we have to download the current timestamp from the internet.

### Status message protocol

As mentioned [above](#connectivity), the MQTT protocol was chosen to be the layer for transferring reports. Its payload is binary so we
could implement a custom format of messages and... but why would anyone do that, when there's multiple serialization format available,
supported and battle proven?  
I have some experience with [Google Protocol Buffer](https://developers.google.com/protocol-buffers) and suprisingly, there exists a
[plain-C implementation](https://jpa.kapsi.fi/nanopb/) targeted for embedded systems. Hurray, one thing solved - one just needs to download
the compiler and include few files in the project.

### Remote switch connectivity

Where ESP32 features a WiFi and BL (even BLE) too, both these are quite power-consuming and are unable to work when the device is sleeping.
A second option is to use 433 Mhz radio, which has a big disadvantage - it's totally unreliable (think of it like UDP). Our wild card is
well known 2.4 GHz radio nRF24, which is more user-friendly and it's transmission is reliable (think of it like TCP) so there's no problem
with transferring larger payloads in an environment with bigger interference.  
The [RF24 library](http://maniacbug.github.io/RF24/) is used for the nRF24 controlling.

### Remote switch protocol and security

A communication between the device and [remote switch](/module-remote) is done over the nRF24 chip. It can send any binary payload up to 32
bytes. We could send simple `'hello'` but... surely there's some attacker waiting for us and trying to hack our device! So, we need to put
some security in place.  
When you google it, you probably find a term [_rolling code_](https://en.wikipedia.org/wiki/Rolling_code). Based on the principles, I
decided to go with a simple number-incrementing of some number, where the next number has to be bigger than the previous. So here there are
two more questions:

1. How to code will be transmitted?
1. How to prevent its abuse?

Even though the protocol could just send a binary-serialized (or text ;-)) version of the code, something more future-proof seems like a
good idea - so why not to use [GPB again](#status-message-protocol)?  
As for the abuse prevention, we have to implement some encryption. Where AES may look like a good default as it's just everywhere, it may be
too heavy for this use-case and it's also a block cipher - so the output is either 16 or 32 bytes, which we don't want. In that case the
encryption key would have to be the same for all remote switches and that would lower the security. Fortunately, there exist a page called
[Arduino Cryptography Library](https://rweather.github.io/arduinolibs/crypto.html) and it points to quite new yet powerful encryption called
[ASCON128](https://ascon.iaik.tugraz.at) which is an authenticated cipher - it proves authenticity of the sender and consistency of the
whole message.

In our case, it means [this](https://github.com/alarm-garage/alarm-garage/blob/master/src/alarm.proto#L20):

```protobuf
message RemoteSignal {
  required string client_id = 1 [(nanopb).max_length = 4];
  required bytes payload = 2 [(nanopb).max_size = 14];
  required bytes auth_tag = 3 [(nanopb).max_size = 8];
}

message RemoteSignalPayload {
  required uint32 code = 1;
  required bytes random = 2 [(nanopb).max_size = 8];
}
```

[`RemoteSignalPayload`](https://github.com/alarm-garage/alarm-garage/blob/master/src/alarm.proto#L26) is fed with the code (the real
payload...) and with some random bytes (to make the decryption harder) and encrypted using `authData`, `key` and `iv`
into `RemoteSignal.payload`. `authData` and `key` are custom for all clients (remote switches) and are part of the device configuration.
They are selected base on the `RemoteSignal.client_id`. `iv` is common for all the clients and is again part of the
configuration. `auth_tag` is calculated for each encrypted message and is verified by the receiver (think of it as MAC).

Because the [remote switch](/module-remote) needs to be sure that some action was actually initiated, it receives a confirmation (modelled
as [`RemoteSignalResponse`](https://github.com/alarm-garage/alarm-garage/blob/master/src/alarm.proto#L31)) is sent back to it. This can't be
done by ACK payload (supported by the [RF24 library](http://maniacbug.github.io/RF24/)) because that has to be prepared in advance (which
creates kind of chicken-egg problem) so we have to manually

1. Switch the main module to transmitter mode
1. Switch the remote controller to receiver mode for a moment necessary to transmit the full response back

The same encryption etc. is used for the response; in fact, the response message is very similar to the "request" one and contains only a
single field in addition.

Both the request and response messages are designed to fit 32 B capacity of the transmission under every circumstances (in fact, only the
code itself has variable length and everything else has fixed size) and we're using
the [dynamic payload](http://maniacbug.github.io/RF24/classRF24.html#a443888504975d7441d6452a09d09a8fa) functionality.

---

The code is very simple:

1. In the setup, it connects to the SIM808 modem (GSM/data network, GPS is not used at all.. we're in a garage) and to the MQTT.

TODO

[Networking.cpp]: https://github.com/alarm-garage/alarm-garage-server/src/Networking.cpp
