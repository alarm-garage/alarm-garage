#include "RollingCodeManager.h"

RollingCodeManager::RollingCodeManager() {
    this->clients = new SimpleMap<String, ClientState>([](String &a, String &b) -> int {
        if (a == b) return 0;
        if (a > b) return 1;
        return -1;
    });
}

bool RollingCodeManager::validate(byte *rawSignal) {
    std::lock_guard<std::mutex> lg(mutex);

    pb_istream_t stream = pb_istream_from_buffer(rawSignal, AG_RADIO_PAYLOAD_SIZE);

    _protocol_RemoteSignal signal = protocol_RemoteSignal_init_zero;

    if (!pb_decode(&stream, protocol_RemoteSignal_fields, &signal)) {
        printf("Decoding signal failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }

    const String &clientId = String(signal.client_id);
    if (!this->clients->has(clientId)) {
        Serial.printf("Could not find client ID %s\n", clientId.c_str());
        return false;
    }

    ClientState clientState = clients->get(clientId);

    // reuse the rawSignal, orig. content won't be needed anymore
    if (!decrypt_signal(rawSignal, clientState.key, clientState.authData, &signal)) {
        Serial.println("Could not decrypt the signal!");
        return false;
    }

    pb_istream_t stream2 = pb_istream_from_buffer(rawSignal, AG_SIGNAL_DATA_SIZE);
    _protocol_RemoteSignalPayload signalPayload = protocol_RemoteSignalPayload_init_zero;

    if (!pb_decode(&stream2, protocol_RemoteSignalPayload_fields, &signalPayload)) {
        printf("Decoding signal payload failed: %s\n", PB_GET_ERROR(&stream2));
        return false;
    }

    uint32_t requiredCode = clientState.lastCode + 1;
    bool success = signalPayload.code >= requiredCode;

    if (success) {
        clientState.lastCode = signalPayload.code;
        clients->put(clientId, clientState);
        persistStates();
    } else {
        Serial.printf("Received code %d, required >= %d, clientId %s\n", signalPayload.code, requiredCode, clientId.c_str());
    }

    return success;
}

bool RollingCodeManager::decrypt_signal(byte *decrypted, const byte *key, const byte *authData, const _protocol_RemoteSignal *signal) {
    cipher.clear();

    if (!cipher.setKey(key, AG_SIGNAL_KEY_SIZE)) {
        return false;
    }
    if (!cipher.setIV(iv, AG_SIGNAL_IV_SIZE)) {
        return false;
    }

    cipher.addAuthData(authData, AG_SIGNAL_AUTH_SIZE);
    cipher.decrypt(decrypted, signal->payload.bytes, AG_SIGNAL_DATA_SIZE);
    return cipher.checkTag(signal->auth_tag.bytes, AG_SIGNAL_AUTH_TAG_SIZE);
}

bool RollingCodeManager::loadStates() {
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

bool RollingCodeManager::persistStates() {
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

bool RollingCodeManager::init() {
    return loadStates();
}
