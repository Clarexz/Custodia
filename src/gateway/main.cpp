#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

namespace {
// Pines UART hacia el Solar Node
constexpr int SOLAR_UART_RX_PIN = 17;  // LilyGo GPIO17 ← Solar D7
constexpr int SOLAR_UART_TX_PIN = 18;  // LilyGo GPIO18 → Solar D6
constexpr uint32_t SOLAR_UART_BAUD = 115200;

// Temporizadores y límites del protocolo
constexpr unsigned long PING_INTERVAL_MS = 5000;
constexpr unsigned long UART_READ_TIMEOUT_MS = 10000;
constexpr unsigned long RESEND_WAIT_MS = 1500;

// Wi-Fi (rellenar con credenciales reales antes de campo)
constexpr char WIFI_SSID[] = "Totalplay-2.4G-f128";
constexpr char WIFI_PASS[] = "sAZBHbpPzub6xpyU";

// Endpoint HTTP de pruebas (fallback Wi-Fi)
constexpr char HTTP_POST_URL[] = "https://webhook.site/8eafb7bc-1464-4751-b4d4-f7fe718f1603";

enum class GatewayState : uint8_t {
    Idle,
    WaitingBatch,
    ReceivingBatch,
    ProcessingBatch
};

struct BatchSession {
    uint16_t sessionId = 0;
    size_t expectedRecords = 0;
    size_t expectedBytes = 0;
    size_t receivedBytes = 0;
    std::vector<String> records;
    std::vector<bool> receivedMask;
    unsigned long lastAction = 0;
    bool active = false;

    void reset() {
        sessionId = 0;
        expectedRecords = 0;
        expectedBytes = 0;
        receivedBytes = 0;
        records.clear();
        receivedMask.clear();
        lastAction = 0;
        active = false;
    }

    bool isComplete() const {
        if (!active) return false;
        for (bool flag : receivedMask) {
            if (!flag) return false;
        }
        return true;
    }
};

HardwareSerial solarLink(1);
GatewayState currentState = GatewayState::Idle;
BatchSession currentBatch;

String rxBuffer;
unsigned long lastPing = 0;

// Funciones utilitarias
void logLine(const String& line) {
    Serial.println(line);
}

void sendToSolar(const String& line) {
    solarLink.print(line);
    solarLink.print('\n');
    Serial.println("[GATEWAY] UART >>> " + line);
}

void sendPing() {
    sendToSolar("PING");
    lastPing = millis();
}

bool ensureWiFiConnected() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    logLine("[GATEWAY] Conectando a Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
        logLine("[GATEWAY] Wi-Fi conectado: " + WiFi.localIP().toString());
        return true;
    }

    logLine("[GATEWAY] WARN: No se pudo conectar a Wi-Fi.");
    return false;
}

bool attemptCellularUpload(const BatchSession& batch) {
    // Stub celular: aún no implementado, retornamos fallo para forzar fallback.
    (void)batch;
    logLine("[GATEWAY] Simulando intento celular... fallo esperado.");
    return false;
}

bool postBatchOverWiFi(const BatchSession& batch) {
    if (!ensureWiFiConnected()) {
        return false;
    }

    HTTPClient client;
    if (!client.begin(HTTP_POST_URL)) {
        logLine("[GATEWAY] ERROR: HTTPClient begin falló.");
        return false;
    }

    client.addHeader("Content-Type", "application/json");

    String payload = "{\"session\":" + String(batch.sessionId) + "},\"records\":[";
    for (size_t i = 0; i < batch.records.size(); ++i) {
        const String& entry = batch.records[i];
        payload += "\"" + entry + "\"";
        if (i + 1 < batch.records.size()) {
            payload += ",";
        }
    }
    payload += "]}";

    int httpCode = client.POST(payload);
    if (httpCode < 0) {
        logLine("[GATEWAY] ERROR: HTTP POST falló: " + String(client.errorToString(httpCode)));
        client.end();
        return false;
    }

    logLine("[GATEWAY] HTTP POST code=" + String(httpCode));
    client.end();
    return httpCode >= 200 && httpCode < 300;
}

String hexDecode(const String& hex) {
    String out;
    out.reserve(hex.length() / 2);
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        char c1 = hex.charAt(i);
        char c2 = hex.charAt(i + 1);
        auto decodeNibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + c - 'A');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + c - 'a');
            return 0;
        };
        uint8_t val = (decodeNibble(c1) << 4) | decodeNibble(c2);
        out += static_cast<char>(val);
    }
    return out;
}

void resetStateToIdle() {
    currentBatch.reset();
    currentState = GatewayState::Idle;
    lastPing = millis();
}

void processIdleState() {
    const unsigned long now = millis();
    if (now - lastPing >= PING_INTERVAL_MS) {
        sendPing();
    }
}

void startBatch(uint16_t sessionId, size_t count, size_t bytes) {
    currentBatch.reset();
    currentBatch.sessionId = sessionId;
    currentBatch.expectedRecords = count;
    currentBatch.expectedBytes = bytes;
    currentBatch.records.resize(count);
    currentBatch.receivedMask.assign(count, false);
    currentBatch.active = true;
    currentBatch.lastAction = millis();

    logLine("[GATEWAY] START_BATCH recibido. Sesión " + String(sessionId) +
            " registros=" + String(count) + " bytes=" + String(bytes));

    sendToSolar("ACK:" + String(sessionId));
    currentState = GatewayState::ReceivingBatch;
}

void handleDataFrame(uint16_t sessionId, size_t index, size_t len, const String& hexPayload) {
    if (!currentBatch.active || sessionId != currentBatch.sessionId) {
        logLine("[GATEWAY] WARN: DATA con sesión inválida. Enviando CANCEL.");
        sendToSolar("CANCEL:" + String(sessionId));
        resetStateToIdle();
        return;
    }

    if (index >= currentBatch.expectedRecords) {
        logLine("[GATEWAY] WARN: Índice fuera de rango. Cancelando sesión.");
        sendToSolar("CANCEL:" + String(sessionId));
        resetStateToIdle();
        return;
    }

    String decoded = hexDecode(hexPayload);
    if (decoded.length() != len) {
        logLine("[GATEWAY] WARN: Longitud payload inconsistente. Solicitando RESEND.");
        sendToSolar("RESEND:" + String(sessionId) + ":" + String(index));
        return;
    }

    currentBatch.records[index] = decoded;
    currentBatch.receivedMask[index] = true;
    currentBatch.receivedBytes += decoded.length();
    currentBatch.lastAction = millis();

    logLine("[GATEWAY] DATA índice " + String(index) + " recibido (" + String(len) + " bytes).");
}

void handleEndBatch(uint16_t sessionId) {
    if (!currentBatch.active || sessionId != currentBatch.sessionId) {
        sendToSolar("CANCEL:" + String(sessionId));
        resetStateToIdle();
        return;
    }

    if (!currentBatch.isComplete()) {
        logLine("[GATEWAY] WARN: Lote incompleto al recibir END_BATCH.");
        for (size_t i = 0; i < currentBatch.receivedMask.size(); ++i) {
            if (!currentBatch.receivedMask[i]) {
                sendToSolar("RESEND:" + String(sessionId) + ":" + String(i));
                return;
            }
        }
    }

    if (currentBatch.receivedBytes != currentBatch.expectedBytes) {
        logLine("[GATEWAY] WARN: Bytes recibidos no coinciden. Actual=" +
                String(currentBatch.receivedBytes) + " esperado=" + String(currentBatch.expectedBytes));
    }

    logLine("[GATEWAY] END_BATCH recibido. Pasando a procesamiento.");
    currentState = GatewayState::ProcessingBatch;
}

void processBatch() {
    if (!currentBatch.active) {
        resetStateToIdle();
        return;
    }

    logLine("[GATEWAY] Procesando lote. Registros=" + String(currentBatch.records.size()));

    bool success = attemptCellularUpload(currentBatch);
    if (!success) {
        success = postBatchOverWiFi(currentBatch);
    }

    if (success) {
        sendToSolar("TRANSFER_OK:" + String(currentBatch.sessionId));
        logLine("[GATEWAY] TRANSFER_OK enviado.");
    } else {
        sendToSolar("TRANSFER_FAIL:" + String(currentBatch.sessionId) + ":NET_ERROR");
        logLine("[GATEWAY] TRANSFER_FAIL enviado.");
    }

    resetStateToIdle();
}

void handleIncomingLine(const String& line) {
    if (line == "IDLE") {
        // Sin datos desde el Solar Node.
        return;
    }

    int firstColon = line.indexOf(':');
    if (firstColon < 0) {
        if (line == "PING") {
            // No esperamos PING entrante desde Solar, lo ignoramos.
        } else {
            logLine("[GATEWAY] WARN: Línea desconocida: " + line);
        }
        return;
    }

    String command = line.substring(0, firstColon);
    String payload = line.substring(firstColon + 1);

    if (command == "START_BATCH") {
        int colon2 = payload.indexOf(':');
        int colon3 = payload.indexOf(':', colon2 + 1);
        if (colon2 < 0 || colon3 < 0) {
            logLine("[GATEWAY] WARN: START_BATCH mal formado.");
            return;
        }
        uint16_t session = payload.substring(0, colon2).toInt();
        size_t count = payload.substring(colon2 + 1, colon3).toInt();
        size_t bytes = payload.substring(colon3 + 1).toInt();
        startBatch(session, count, bytes);
    } else if (command == "DATA") {
        int colon2 = payload.indexOf(':');
        int colon3 = payload.indexOf(':', colon2 + 1);
        int colon4 = payload.indexOf(':', colon3 + 1);
        if (colon2 < 0 || colon3 < 0 || colon4 < 0) {
            logLine("[GATEWAY] WARN: DATA mal formado.");
            return;
        }

        uint16_t session = payload.substring(0, colon2).toInt();
        size_t index = payload.substring(colon2 + 1, colon3).toInt();
        size_t len = payload.substring(colon3 + 1, colon4).toInt();
        String hexPayload = payload.substring(colon4 + 1);
        handleDataFrame(session, index, len, hexPayload);
    } else if (command == "END_BATCH") {
        uint16_t session = payload.toInt();
        handleEndBatch(session);
    } else {
        logLine("[GATEWAY] WARN: Comando no reconocido: " + command);
    }
}

void readFromSolar() {
    while (solarLink.available()) {
        char c = solarLink.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            String line = rxBuffer;
            rxBuffer = "";
            line.trim();
            if (line.length() > 0) {
                Serial.println("[GATEWAY] UART <<< " + line);
                handleIncomingLine(line);
            }
        } else {
            rxBuffer += c;
            if (rxBuffer.length() > 256) {
                rxBuffer = "";
            }
        }
    }
}

void runStateMachine() {
    switch (currentState) {
        case GatewayState::Idle:
            processIdleState();
            break;
        case GatewayState::ReceivingBatch:
            // Si pasa demasiado tiempo sin datos, cancelar y volver a idle.
            if (currentBatch.active &&
                (millis() - currentBatch.lastAction) > UART_READ_TIMEOUT_MS) {
                logLine("[GATEWAY] WARN: Timeout de recepción. Cancelando sesión.");
                sendToSolar("CANCEL:" + String(currentBatch.sessionId));
                resetStateToIdle();
            }
            break;
        case GatewayState::ProcessingBatch:
            processBatch();
            break;
        case GatewayState::WaitingBatch:
            // Ya no se usa este estado, se mantiene para compatibilidad.
            currentState = GatewayState::ReceivingBatch;
            break;
    }
}

void initializeConsole() {
    Serial.begin(115200);
    const unsigned long start = millis();
    while (!Serial && (millis() - start) < 4000) {
        delay(10);
    }
    Serial.println();
    Serial.println(F("[GATEWAY] Iniciando gateway store-and-forward..."));
}

void initializeSolarLink() {
    solarLink.begin(SOLAR_UART_BAUD, SERIAL_8N1, SOLAR_UART_RX_PIN, SOLAR_UART_TX_PIN);
    Serial.printf("[GATEWAY] UART listo en RX=%d TX=%d @ %lu bps\n",
                  SOLAR_UART_RX_PIN, SOLAR_UART_TX_PIN, static_cast<unsigned long>(SOLAR_UART_BAUD));
    lastPing = millis();
}
}  // namespace

void setup() {
    initializeConsole();
    initializeSolarLink();
    WiFi.mode(WIFI_STA);
}

void loop() {
    readFromSolar();
    runStateMachine();
    delay(10);
}
