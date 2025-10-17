/*
 * END_NODE_REPEATER_ROLE.H - Lógica para el rol END_NODE_REPEATER
 */

#ifndef END_NODE_REPEATER_ROLE_H
#define END_NODE_REPEATER_ROLE_H

#include <Arduino.h>
#include <vector>

/*
 * CLASE PARA MANEJO DEL ROL END_NODE_REPEATER
 *
 * Nota: Implementación inicial mínima. Se ampliará con almacenamiento
 * de paquetes LoRa y transferencia UART en pasos posteriores.
 */
class EndNodeRepeaterRole {
    enum class TransferState : uint8_t {
        Idle,
        WaitingAck,
        SendingData,
        AwaitingResult
    };

public:
    static constexpr size_t MAX_LOG_ENTRIES = 512;

    EndNodeRepeaterRole();
    ~EndNodeRepeaterRole();

    void handleMode();
    void recordLoRaPacket(uint16_t sourceID,
                          float latitude,
                          float longitude,
                          uint32_t timestamp,
                          uint16_t voltageMilli,
                          float rssi,
                          float snr);

    size_t getStoredCount() const { return storedCount; }
    bool hasPendingData() const { return storedCount > 0; }

private:
    bool announced;
    bool initialized;
    bool storageReady;
    bool uartReady;
    unsigned long lastStatusLog;
    unsigned long lastStorageRetry;
    unsigned long lastDataSend;
    unsigned long lastBatchAnnounce;
    unsigned long resultWaitStart;
    size_t storedCount;
    uint16_t currentSessionId;
    uint16_t nextSessionId;
    size_t nextRecordIndex;
    size_t batchTotalBytes;
    bool resendPending;
    size_t resendIndex;
    uint8_t announceAttempts;
    TransferState transferState;
    std::vector<String> batchRecords;
    String serialBuffer;

    bool ensureInitialized();
    bool ensureSerialReady();
    void loadExistingLog();
    void createLogFile();
    bool appendRecord(const String& line);
    void pruneLogIfNeeded();
    bool loadBatchFromLog();
    void startBatchTransfer();
    void resetTransfer(bool preserveData);
    void processGatewayInput();
    void handleGatewayLine(const String& line);
    void handlePing();
    void handleAck(uint16_t session);
    void handleTransferOk(uint16_t session);
    void handleTransferFail(uint16_t session, const String& reason);
    void handleResend(uint16_t session, size_t index);
    void handleCancel(uint16_t session);
    void sendLine(const String& line);
    void sendIdleResponse();
    void sendStartBatch();
    void sendNextRecord();
    void sendEndBatch();
    static String hexEncode(const String& input);
};

extern EndNodeRepeaterRole endNodeRepeaterRole;

#endif
