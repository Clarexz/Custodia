/*
 * END_NODE_REPEATER_ROLE.CPP - Lógica de almacenamiento y transferencia UART
 */

#include "end_node_repeater_role.h"
#include "../config/config_manager.h"

#if !CONFIG_MANAGER_HAS_PREFERENCES
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;
#endif

namespace {
#if !CONFIG_MANAGER_HAS_PREFERENCES
constexpr char LOG_FILE_PATH[] = "/lora_log.csv";
constexpr char LOG_FILE_HEADER[] = "timestamp,source_id,latitude,longitude,voltage_mV,rssi_dBm,snr_dB";
#endif

constexpr uint32_t GATEWAY_BAUD = 115200;
constexpr unsigned long STATUS_INTERVAL_MS = 10000;
constexpr unsigned long STORAGE_RETRY_INTERVAL_MS = 5000;
constexpr unsigned long DATA_SEND_INTERVAL_MS = 20;
constexpr unsigned long ACK_TIMEOUT_MS = 2000;
constexpr unsigned long RESULT_TIMEOUT_MS = 5000;
constexpr uint8_t MAX_START_RETRIES = 3;

HardwareSerial& gatewaySerial = Serial1;
}  // namespace

EndNodeRepeaterRole endNodeRepeaterRole;

EndNodeRepeaterRole::EndNodeRepeaterRole()
    : announced(false),
      initialized(false),
      storageReady(false),
      uartReady(false),
      lastStatusLog(0),
      lastStorageRetry(0),
      lastDataSend(0),
      lastBatchAnnounce(0),
      resultWaitStart(0),
      storedCount(0),
      currentSessionId(0),
      nextSessionId(1),
      nextRecordIndex(0),
      batchTotalBytes(0),
      resendPending(false),
      resendIndex(0),
      announceAttempts(0),
      transferState(TransferState::Idle),
      batchRecords(),
      serialBuffer("") {}

EndNodeRepeaterRole::~EndNodeRepeaterRole() = default;

bool EndNodeRepeaterRole::ensureInitialized() {
#if CONFIG_MANAGER_HAS_PREFERENCES
    storageReady = false;
    return false;
#else
    if (storageReady) {
        return true;
    }

    if (!initialized) {
        initialized = true;
    }

    storageReady = InternalFS.begin();
    if (!storageReady) {
        Serial.println("[END_NODE] ERROR: No se pudo montar InternalFS.");
        return false;
    }

    if (!InternalFS.exists(LOG_FILE_PATH)) {
        createLogFile();
    } else {
        loadExistingLog();
    }
    return storageReady;
#endif
}

bool EndNodeRepeaterRole::ensureSerialReady() {
    if (uartReady) {
        return true;
    }

    // Forzar el uso de Serial1. La comprobación en tiempo de compilación
    // (#if defined) falla incorrectamente para la variante de placa personalizada.
    // Como este rol es solo para el Solar Node, podemos asumir que Serial1 existe.
    Serial1.begin(GATEWAY_BAUD);
    uartReady = true;
    Serial.println("[END_NODE] UART con gateway inicializado @115200.");
    return uartReady;
}

void EndNodeRepeaterRole::createLogFile() {
#if !CONFIG_MANAGER_HAS_PREFERENCES
    InternalFS.remove(LOG_FILE_PATH);

    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_WRITE)) {
        storageReady = false;
        Serial.println("[END_NODE] ERROR: No se pudo crear el archivo de log.");
        return;
    }
    file.println(LOG_FILE_HEADER);
    file.close();
    storedCount = 0;
#endif
}

void EndNodeRepeaterRole::loadExistingLog() {
#if !CONFIG_MANAGER_HAS_PREFERENCES
    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_READ)) {
        Serial.println("[END_NODE] WARN: No se pudo abrir log existente, recreando archivo.");
        createLogFile();
        return;
    }

    storedCount = 0;
    bool headerSkipped = false;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        line.trim();
        if (line.length() == 0) {
            continue;
        }
        storedCount++;
    }
    file.close();

    if (storedCount > MAX_LOG_ENTRIES) {
        pruneLogIfNeeded();
    }
#endif
}

bool EndNodeRepeaterRole::appendRecord(const String& line) {
#if CONFIG_MANAGER_HAS_PREFERENCES
    (void)line;
    return false;
#else
    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_WRITE)) {
        Serial.println("[END_NODE] ERROR: No se pudo abrir log para escritura.");
        storageReady = false;
        return false;
    }

    size_t written = file.println(line);
    file.close();

    if (written == 0) {
        Serial.println("[END_NODE] ERROR: Fallo al escribir registro en log.");
        return false;
    }

    storedCount++;
    pruneLogIfNeeded();
    return true;
#endif
}

void EndNodeRepeaterRole::pruneLogIfNeeded() {
#if !CONFIG_MANAGER_HAS_PREFERENCES
    if (storedCount <= MAX_LOG_ENTRIES) {
        return;
    }

    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_READ)) {
        Serial.println("[END_NODE] WARN: No se pudo abrir log para poda.");
        storageReady = false;
        return;
    }

    bool headerSkipped = false;
    std::vector<String> tail;
    tail.reserve(MAX_LOG_ENTRIES);

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        line.trim();
        if (line.length() == 0) {
            continue;
        }
        tail.push_back(line);
        if (tail.size() > MAX_LOG_ENTRIES) {
            tail.erase(tail.begin());
        }
    }
    file.close();

    InternalFS.remove(LOG_FILE_PATH);
    File writer(InternalFS);
    if (!writer.open(LOG_FILE_PATH, FILE_O_WRITE)) {
        Serial.println("[END_NODE] ERROR: No se pudo reescribir log al podar.");
        storageReady = false;
        return;
    }

    writer.println(LOG_FILE_HEADER);
    for (const auto& line : tail) {
        writer.println(line);
    }
    writer.close();

    storedCount = tail.size();
#endif
}

bool EndNodeRepeaterRole::loadBatchFromLog() {
#if CONFIG_MANAGER_HAS_PREFERENCES
    return false;
#else
    batchRecords.clear();
    batchTotalBytes = 0;

    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_READ)) {
        Serial.println("[END_NODE] ERROR: No se pudo abrir log para lectura.");
        storageReady = false;
        return false;
    }

    bool headerSkipped = false;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        line.trim();
        if (line.length() == 0) {
            continue;
        }
        batchRecords.push_back(line);
        batchTotalBytes += line.length();
    }
    file.close();

    storedCount = batchRecords.size();
    return !batchRecords.empty();
#endif
}

void EndNodeRepeaterRole::recordLoRaPacket(uint16_t sourceID,
                                           float latitude,
                                           float longitude,
                                           uint32_t timestamp,
                                           uint16_t voltageMilli,
                                           float rssi,
                                           float snr) {
    if (!ensureInitialized()) {
        return;
    }

#if CONFIG_MANAGER_HAS_PREFERENCES
    (void)sourceID;
    (void)latitude;
    (void)longitude;
    (void)timestamp;
    (void)voltageMilli;
    (void)rssi;
    (void)snr;
    return;
#else
    String line;
    line.reserve(96);
    line += String(timestamp);
    line += ",";
    line += String(sourceID);
    line += ",";
    line += String(latitude, 6);
    line += ",";
    line += String(longitude, 6);
    line += ",";
    line += String(voltageMilli);
    line += ",";
    line += String(rssi, 2);
    line += ",";
    line += String(snr, 2);

    appendRecord(line);
#endif
}

void EndNodeRepeaterRole::handleMode() {
    unsigned long now = millis();

    if (!announced) {
        Serial.println("[END_NODE] Rol END_NODE_REPEATER activo.");
        Serial.println("[END_NODE] Activando almacenamiento de packets LoRa (límite 512 registros).");
        announced = true;
    }

    ensureInitialized();
    ensureSerialReady();

    if (!storageReady) {
        if (now - lastStorageRetry >= STORAGE_RETRY_INTERVAL_MS) {
            lastStorageRetry = now;
            ensureInitialized();
        }
        delay(100);
        return;
    }

    processGatewayInput();

    if (transferState == TransferState::WaitingAck &&
        announceAttempts > 0 &&
        (millis() - lastBatchAnnounce) > ACK_TIMEOUT_MS) {
        if (announceAttempts < MAX_START_RETRIES) {
            sendStartBatch();
        } else {
            Serial.println("[END_NODE] WARN: Timeout esperando ACK, reintentará más tarde.");
            resetTransfer(true);
        }
    }

    if (transferState == TransferState::SendingData) {
        if ((millis() - lastDataSend) >= DATA_SEND_INTERVAL_MS) {
            sendNextRecord();
        }
    } else if (transferState == TransferState::AwaitingResult &&
               resultWaitStart > 0 &&
               (millis() - resultWaitStart) > RESULT_TIMEOUT_MS) {
        Serial.println("[END_NODE] WARN: Timeout esperando TRANSFER_OK/FAIL.");
        resetTransfer(true);
    }

    if (now - lastStatusLog >= STATUS_INTERVAL_MS) {
        lastStatusLog = now;
        Serial.println("[END_NODE] Packets almacenados: " + String(storedCount) +
                       "/" + String(MAX_LOG_ENTRIES));
        if (transferState != TransferState::Idle) {
            Serial.println("[END_NODE] Estado transferencia activo, sesión " + String(currentSessionId));
        }
    }

    delay(20);
}

void EndNodeRepeaterRole::processGatewayInput() {
    if (!uartReady) {
        return;
    }

    while (gatewaySerial.available()) {
        char c = gatewaySerial.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            String line = serialBuffer;
            serialBuffer = "";
            line.trim();
            if (line.length() > 0) {
                handleGatewayLine(line);
            }
        } else {
            serialBuffer += c;
            if (serialBuffer.length() > 256) {
                serialBuffer.remove(0);
            }
        }
    }
}

void EndNodeRepeaterRole::handleGatewayLine(const String& line) {
    if (line == "PING") {
        handlePing();
        return;
    }

    if (line == "IDLE" || line == "BUSY") {
        // Estas respuestas no requieren acción.
        return;
    }

    int colon = line.indexOf(':');
    if (colon < 0) {
        Serial.println("[END_NODE] WARN: Comando desconocido del gateway: " + line);
        return;
    }

    String command = line.substring(0, colon);
    String payload = line.substring(colon + 1);

    if (command == "ACK") {
        handleAck(payload.toInt());
    } else if (command == "TRANSFER_OK") {
        handleTransferOk(payload.toInt());
    } else if (command == "TRANSFER_FAIL") {
        int secondColon = payload.indexOf(':');
        uint16_t session = payload.toInt();
        String reason = (secondColon > 0) ? payload.substring(secondColon + 1) : "UNKNOWN";
        handleTransferFail(session, reason);
    } else if (command == "RESEND") {
        int secondColon = payload.indexOf(':');
        if (secondColon < 0) {
            Serial.println("[END_NODE] WARN: RESEND malformado.");
            return;
        }
        uint16_t session = payload.substring(0, secondColon).toInt();
        size_t index = payload.substring(secondColon + 1).toInt();
        handleResend(session, index);
    } else if (command == "CANCEL") {
        handleCancel(payload.toInt());
    } else {
        Serial.println("[END_NODE] WARN: Comando no soportado: " + command);
    }
}

void EndNodeRepeaterRole::handlePing() {
    if (transferState != TransferState::Idle) {
        sendLine("BUSY");
        return;
    }

    if (!storageReady || storedCount == 0) {
        sendIdleResponse();
        return;
    }

    if (!loadBatchFromLog()) {
        sendIdleResponse();
        return;
    }

    startBatchTransfer();
}

void EndNodeRepeaterRole::startBatchTransfer() {
    currentSessionId = nextSessionId++;
    if (nextSessionId == 0) {
        nextSessionId = 1;
    }

    transferState = TransferState::WaitingAck;
    nextRecordIndex = 0;
    resendPending = false;
    announceAttempts = 0;
    lastBatchAnnounce = 0;
    resultWaitStart = 0;

    Serial.println("[END_NODE] Iniciando transferencia. Sesión " + String(currentSessionId) +
                   " con " + String(batchRecords.size()) + " registros.");
    sendStartBatch();
}

void EndNodeRepeaterRole::sendStartBatch() {
    if (!uartReady) {
        return;
    }
    announceAttempts++;
    lastBatchAnnounce = millis();

    String cmd = "START_BATCH:";
    cmd += String(currentSessionId);
    cmd += ":";
    cmd += String(batchRecords.size());
    cmd += ":";
    cmd += String(batchTotalBytes);

    sendLine(cmd);
}

void EndNodeRepeaterRole::handleAck(uint16_t session) {
    if (transferState != TransferState::WaitingAck || session != currentSessionId) {
        sendLine("CANCEL:" + String(session));
        return;
    }

    transferState = TransferState::SendingData;
    resendPending = false;
    lastDataSend = 0;
    Serial.println("[END_NODE] ACK recibido. Enviando lote...");
}

void EndNodeRepeaterRole::sendNextRecord() {
    if (transferState != TransferState::SendingData || !uartReady) {
        return;
    }

    size_t index = resendPending ? resendIndex : nextRecordIndex;
    if (index >= batchRecords.size()) {
        transferState = TransferState::AwaitingResult;
        sendEndBatch();
        return;
    }

    const String& record = batchRecords[index];
    String payloadHex = hexEncode(record);

    String cmd = "DATA:";
    cmd += String(currentSessionId);
    cmd += ":";
    cmd += String(index);
    cmd += ":";
    cmd += String(record.length());
    cmd += ":";
    cmd += payloadHex;

    sendLine(cmd);
    lastDataSend = millis();

    if (resendPending) {
        resendPending = false;
    } else {
        nextRecordIndex++;
        if (nextRecordIndex >= batchRecords.size()) {
            transferState = TransferState::AwaitingResult;
            sendEndBatch();
        }
    }
}

void EndNodeRepeaterRole::sendEndBatch() {
    if (!uartReady) {
        return;
    }

    String cmd = "END_BATCH:";
    cmd += String(currentSessionId);
    sendLine(cmd);
    resultWaitStart = millis();
}

void EndNodeRepeaterRole::deleteRecordsFromLog(size_t recordsToDelete) {
#if !CONFIG_MANAGER_HAS_PREFERENCES
    if (recordsToDelete == 0) {
        return;
    }
    if (recordsToDelete >= storedCount) {
        createLogFile();
        return;
    }

    File file(InternalFS);
    if (!file.open(LOG_FILE_PATH, FILE_O_READ)) {
        Serial.println("[END_NODE] WARN: No se pudo abrir log para eliminación, recreando.");
        createLogFile();
        return;
    }

    std::vector<String> remainingRecords;
    remainingRecords.reserve(storedCount - recordsToDelete);

    bool headerSkipped = false;
    size_t skippedRecords = 0;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        line.trim();
        if (line.length() == 0) {
            continue;
        }

        if (skippedRecords < recordsToDelete) {
            skippedRecords++;
            continue;
        }
        remainingRecords.push_back(line);
    }
    file.close();

    InternalFS.remove(LOG_FILE_PATH);
    File writer(InternalFS);
    if (!writer.open(LOG_FILE_PATH, FILE_O_WRITE)) {
        Serial.println("[END_NODE] ERROR: No se pudo reescribir log tras eliminar.");
        storageReady = false;
        storedCount = 0; // Perdimos el log
        return;
    }

    writer.println(LOG_FILE_HEADER);
    for (const auto& record : remainingRecords) {
        writer.println(record);
    }
    writer.close();

    storedCount = remainingRecords.size();
    Serial.println("[END_NODE] " + String(recordsToDelete) + " registros eliminados. " +
                   String(storedCount) + " restantes.");
#endif
}

void EndNodeRepeaterRole::handleTransferOk(uint16_t session) {
    if (transferState != TransferState::AwaitingResult || session != currentSessionId) {
        return;
    }

    Serial.println("[END_NODE] Transferencia exitosa. Limpieza de log.");
    deleteRecordsFromLog(batchRecords.size());
    resetTransfer(true); // Preserva los registros restantes
}

void EndNodeRepeaterRole::handleTransferFail(uint16_t session, const String& reason) {
    if (session != currentSessionId) {
        return;
    }

    Serial.println("[END_NODE] TRANSFER_FAIL (" + String(session) + "): " + reason);
    resetTransfer(true);
}

void EndNodeRepeaterRole::handleResend(uint16_t session, size_t index) {
    if (session != currentSessionId || transferState == TransferState::Idle) {
        return;
    }

    if (index >= batchRecords.size()) {
        Serial.println("[END_NODE] WARN: Índice RESEND fuera de rango.");
        sendLine("CANCEL:" + String(session));
        resetTransfer(true);
        return;
    }

    resendPending = true;
    resendIndex = index;
    transferState = TransferState::SendingData;
    lastDataSend = 0;
    Serial.println("[END_NODE] Reenviando registro #" + String(index));
}

void EndNodeRepeaterRole::handleCancel(uint16_t session) {
    if (session != currentSessionId) {
        return;
    }
    Serial.println("[END_NODE] Gateway canceló la sesión " + String(session));
    resetTransfer(true);
}

void EndNodeRepeaterRole::resetTransfer(bool preserveData) {
    transferState = TransferState::Idle;
    currentSessionId = 0;
    nextRecordIndex = 0;
    resendPending = false;
    resendIndex = 0;
    batchTotalBytes = 0;
    lastDataSend = 0;
    lastBatchAnnounce = 0;
    announceAttempts = 0;
    resultWaitStart = 0;
    serialBuffer = "";
    batchRecords.clear();

    if (preserveData) {
        loadExistingLog();
    } else {
        createLogFile();
    }
}

void EndNodeRepeaterRole::sendIdleResponse() {
    sendLine("IDLE");
}

void EndNodeRepeaterRole::sendLine(const String& line) {
    if (!uartReady) {
        return;
    }
    gatewaySerial.print(line);
    gatewaySerial.print('\n');
    Serial.println("[END_NODE] UART >>> " + line);
}

String EndNodeRepeaterRole::hexEncode(const String& input) {
    static const char HEX_CHARS[] = "0123456789ABCDEF";
    String out;
    out.reserve(input.length() * 2);
    for (size_t i = 0; i < input.length(); i++) {
        uint8_t b = static_cast<uint8_t>(input.charAt(i));
        out += HEX_CHARS[b >> 4];
        out += HEX_CHARS[b & 0x0F];
    }
    return out;
}