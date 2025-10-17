#include <Arduino.h>

// --- INICIO: Código PoC para prueba de UART ---
// Este firmware es una versión simplificada para el LilyGo Gateway.
// Su único propósito es escuchar en el puerto serie (Serial1) y
// retransmitir cualquier mensaje recibido al monitor serie principal (Serial).
// Esto permite validar la conexión física con el Solar Node.

// Pines para la comunicación UART con el Solar Node en la placa LilyGo T-SIM7080-S3
// Documentado en: docs/GATEWAY_UART_PROTOCOL.md
constexpr int SOLAR_UART_RX_PIN = 17; // Conectado al TX (D6) del Solar Node
constexpr int SOLAR_UART_TX_PIN = 18; // Conectado al RX (D7) del Solar Node
constexpr uint32_t SOLAR_UART_BAUD = 115200;

// El puerto serie que se comunicará con el Solar Node
HardwareSerial SolarSerial(1);

void setup() {
  // Iniciar monitor serie para depuración
  Serial.begin(115200);
  // Pequeña espera para asegurar que el monitor serie se conecte
  delay(2000);
  Serial.println("[GATEWAY_POC] Monitor serie inicializado.");

  // Iniciar la comunicación UART con el Solar Node
  SolarSerial.begin(SOLAR_UART_BAUD, SERIAL_8N1, SOLAR_UART_RX_PIN, SOLAR_UART_TX_PIN);
  Serial.println("[GATEWAY_POC] Escuchando en UART1...");
}

void loop() {
  // Si hay datos disponibles desde el Solar Node
  if (SolarSerial.available()) {
    // Leer la línea completa
    String message = SolarSerial.readStringUntil('\n');
    message.trim(); // Limpiar espacios en blanco

    if (message.length() > 0) {
      // Imprimir el mensaje recibido en el monitor serie
      Serial.print("[GATEWAY_POC] Mensaje recibido del Solar Node: ");
      Serial.println(message);
    }
  }
}
// --- FIN: Código PoC para prueba de UART ---