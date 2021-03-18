#include "RollingCodeHandler.h"

byte rawSignal[AG_RADIO_PAYLOAD_SIZE];

RollingCodeHandler::RollingCodeHandler(StateManager &stateManager, Radio &radio) {
    this->stateManager = &stateManager;

    this->clients = new SimpleMap<String, ClientState>([](String &a, String &b) -> int {
        if (a == b) return 0;
        if (a > b) return 1;
        return -1;
    });

    this->radio = &radio;
}

bool RollingCodeHandler::handleRadioSignal() {
    std::lock_guard<std::mutex> lg(mutex);

    if (!radio->radioReceive(rawSignal)) return false;

    // Decode whole signal:

    pb_istream_t stream = pb_istream_from_buffer(rawSignal, AG_RADIO_PAYLOAD_SIZE);

    _protocol_RemoteSignal signal = protocol_RemoteSignal_init_zero;

    if (!pb_decode(&stream, protocol_RemoteSignal_fields, &signal)) {
        printf("Decoding signal failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    // Load related client data:

    const String &clientId = String(signal.client_id);
    if (!this->clients->has(clientId)) {
        Serial.printf("Could not find client ID %s\n", clientId.c_str());
        return false;
    }

    ClientState clientState = clients->get(clientId);

    // Decrypt signal payload:

    // reuse the rawSignal, orig. content won't be needed anymore
    if (!decryptSignal(rawSignal, clientState.key, clientState.authData, &signal)) {
        Serial.println("Could not decrypt the signal!");
        return false;
    }

    pb_istream_t stream2 = pb_istream_from_buffer(rawSignal, AG_SIGNAL_DATA_SIZE);
    _protocol_RemoteSignalPayload signalPayload = protocol_RemoteSignalPayload_init_zero;

    // Decode signal payload data:

    if (!pb_decode(&stream2, protocol_RemoteSignalPayload_fields, &signalPayload)) {
        printf("Decoding signal payload failed: %s\n", PB_GET_ERROR(&stream2));
        return false;
    }

    // Verify validity of the code:

    uint32_t requiredCode = clientState.lastCode + 1;
    bool codeValid = signalPayload.code >= requiredCode;

    if (!codeValid) {
        Serial.printf("Received code %d, required >= %d, clientId %s\n", signalPayload.code, requiredCode, clientId.c_str());
        return false;
    }

    clientState.lastCode = signalPayload.code;
    clients->put(clientId, clientState);
    persistStates();

    // Toggle state:

    stateManager->toggleArmed();

    // Send response back:

    _protocol_RemoteSignalResponse signalResponse = protocol_RemoteSignalResponse_init_zero;

    strcpy(signalResponse.client_id, signal.client_id);

    _protocol_RemoteSignalResponsePayload signalResponsePayload = protocol_RemoteSignalResponsePayload_init_zero;
    signalResponsePayload.code = signalPayload.code;
    signalResponsePayload.success = true;
    esp_fill_random(signalResponsePayload.random.bytes, 8);
    signalResponsePayload.random.size = 8;

    if (!encryptSignalResponsePayload(&signalResponse, clientState.key, clientState.authData, signalResponsePayload)) {
        Serial.println("Could not encrypt signal response payload!");
        return false;
    }

    printf("Sending success response to code %d\n", clientState.lastCode);

    pb_ostream_t stream3 = pb_ostream_from_buffer(rawSignal, AG_RADIO_PAYLOAD_SIZE);

    if (!pb_encode(&stream3, protocol_RemoteSignalResponse_fields, &signalResponse)) {
        Serial.printf("Encoding signal response failed: %s\n", PB_GET_ERROR(&stream3));
        return false;
    }

    // TODO support variable length of encoding
//    Serial.print("Signal response encoded into ");
//    size_t bytes = stream3.bytes_written;
//    Serial.print(bytes);
//    Serial.print(" bytes\n");

    return radio->radioSend(rawSignal);
}

bool RollingCodeHandler::decryptSignal(byte *buff, const byte *key, const byte *authData, const _protocol_RemoteSignal *signal) {
    cipher.clear();

    if (!cipher.setKey(key, AG_SIGNAL_KEY_SIZE)) {
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        return false;
    }

    cipher.addAuthData(authData, AG_SIGNAL_AUTH_SIZE);
    cipher.decrypt(buff, signal->payload.bytes, AG_SIGNAL_DATA_SIZE);
    return cipher.checkTag(signal->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
}

bool RollingCodeHandler::encryptSignalResponsePayload(_protocol_RemoteSignalResponse *output,
                                                      const byte *key,
                                                      const byte *authData,
                                                      _protocol_RemoteSignalResponsePayload payload) {
    cipher.clear();

    uint8_t pb_buffer[16];
    pb_ostream_t stream = pb_ostream_from_buffer(pb_buffer, sizeof(pb_buffer));

    if (!pb_encode(&stream, protocol_RemoteSignalResponsePayload_fields, &payload)) {
        Serial.println("Could not encode the signal payload!");
        return false;
    }

//    Serial.print("Signal response payload encoded into ");
//    size_t bytes = stream.bytes_written;
//    Serial.print(bytes);
//    Serial.print(" bytes\n");

    if (!cipher.setKey(key, AG_SIGNAL_KEY_SIZE)) {
        Serial.print("setKey ");
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        Serial.print("setIV ");
        return false;
    }

    cipher.addAuthData(authData, AG_SIGNAL_AUTH_SIZE);
    cipher.encrypt(output->payload.bytes, pb_buffer, AG_SIGNAL_DATA_SIZE);
    output->payload.size = AG_SIGNAL_DATA_SIZE;
    cipher.computeTag(output->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
    output->auth_tag.size = AG_SIGNAL_AUTH_TAG_SIZE;

    return true;
}

bool RollingCodeHandler::loadStates() {
    StaticJsonDocument<2048> doc;

    File file = SPIFFS.open("/clients.json", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    const DeserializationError error = deserializeJson(doc, file);
    if (error)
        Serial.println(F("Failed to read file, using default configuration"));

    file.close();

    const JsonArray &obj = doc.as<JsonArray>();
    for (JsonObject value : obj) {
        const String clientId = value["clientId"].as<String>();
        const JsonArray &key = value["key"].as<JsonArray>();
        const JsonArray &authData = value["authData"].as<JsonArray>();

        ClientState state = ClientState{};
        state.lastCode = value["lastCode"].as<uint32_t>();

        for (int i = 0; i < key.size(); i++) {
            state.key[i] = key[i];
        }

        for (int i = 0; i < authData.size(); i++) {
            state.authData[i] = authData[i];
        }

        this->clients->put(clientId, state);
    }

    return true;
}

bool RollingCodeHandler::persistStates() {
    StaticJsonDocument<2048> doc;

    for (int i = 0; i < clients->size(); i++) {
        ClientState cl = clients->getData(i);

        JsonObject state = doc.createNestedObject();

        JsonArray stateKey = state.createNestedArray("key");
        for (auto b : cl.key) {
            stateKey.add(b);
        }

        JsonArray authData = state.createNestedArray("authData");
        for (auto b : cl.authData) {
            authData.add(b);
        }

        state["clientId"] = clients->getKey(i);
        state["lastCode"] = cl.lastCode;
    }

    File file = SPIFFS.open("/clients.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    serializeJson(doc, file);

    file.close();

    return true;
}

bool RollingCodeHandler::init() {
    return loadStates();
}
