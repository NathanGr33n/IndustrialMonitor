/*
 * LoRa Sensor Node - Industrial Monitor
 * 
 * Reads sensors and transmits telemetry via LoRa in compact JSON format
 * Uses deep sleep between transmissions for power efficiency
 * 
 * Hardware: ESP32 + SX1276/SX1278 LoRa module + sensors
 * 
 * Wiring:
 *   LoRa Module -> ESP32
 *   SCK  -> GPIO 5
 *   MISO -> GPIO 19
 *   MOSI -> GPIO 27
 *   CS   -> GPIO 18
 *   RST  -> GPIO 14
 *   DIO0 -> GPIO 26
 */

#include <LoRa.h>
#include <ArduinoJson.h>

// ===== CONFIGURATION =====
// Node Identity
const char* MACHINE_ID = "lathe_01";  // Change for each node

// Transmission Interval
#define SLEEP_SECONDS 30  // Send data every 30 seconds

// LoRa Configuration (MUST match gateway)
#define LORA_FREQUENCY 915E6     // US: 915MHz, EU: 868MHz
#define LORA_BANDWIDTH 125E3
#define LORA_SPREADING 7
#define LORA_CODING_RATE 5
#define LORA_TX_POWER 20
#define LORA_SYNC_WORD 0x12

// LoRa Pins
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 14
#define LORA_DIO0 26

// Sensor Pins (example - adjust for your sensors)
#define TEMP_SENSOR_PIN 34   // Analog temperature sensor
#define VIBE_SENSOR_PIN 35   // Analog vibration sensor

// ===== GLOBALS =====
RTC_DATA_ATTR int bootCount = 0;  // Persists across deep sleep

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  bootCount++;
  Serial.println("\n=== LoRa Sensor Node ===");
  Serial.print("Machine ID: ");
  Serial.println(MACHINE_ID);
  Serial.print("Boot count: ");
  Serial.println(bootCount);
  
  // Initialize LoRa
  setupLoRa();
  
  // Read sensors and transmit
  readAndTransmit();
  
  // Enter deep sleep
  enterDeepSleep();
}

// ===== MAIN LOOP =====
void loop() {
  // Not used - we use deep sleep instead
}

// ===== LORA SETUP =====
void setupLoRa() {
  Serial.println("Initializing LoRa...");
  
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    // Try again after short delay
    delay(5000);
    ESP.restart();
  }
  
  LoRa.setSignalBandwidth(LORA_BANDWIDTH);
  LoRa.setSpreadingFactor(LORA_SPREADING);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  
  Serial.println("LoRa initialized");
}

// ===== READ SENSORS AND TRANSMIT =====
void readAndTransmit() {
  Serial.println("Reading sensors...");
  
  // Read sensors (simulated values for demo - replace with real sensor code)
  float temperature = readTemperature();
  float vibration = readVibration();
  int rpm = readRPM();
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  
  Serial.print("Vibration: ");
  Serial.print(vibration, 3);
  Serial.println(" g");
  
  Serial.print("RPM: ");
  Serial.println(rpm);
  
  // Build compact JSON payload
  StaticJsonDocument<128> doc;
  doc["id"] = MACHINE_ID;
  doc["t"] = round(temperature * 10) / 10.0;  // 1 decimal place
  doc["v"] = round(vibration * 1000) / 1000.0;  // 3 decimal places
  doc["r"] = rpm;
  
  // Serialize to string
  char payload[128];
  serializeJson(doc, payload);
  
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("Payload size: ");
  Serial.print(strlen(payload));
  Serial.println(" bytes");
  
  // Transmit via LoRa
  Serial.println("Transmitting...");
  
  LoRa.beginPacket();
  LoRa.print(payload);
  int result = LoRa.endPacket();
  
  if (result) {
    Serial.println("✓ Transmission successful");
  } else {
    Serial.println("✗ Transmission failed");
  }
  
  delay(100);  // Brief delay for transmission to complete
}

// ===== SENSOR READING FUNCTIONS =====

float readTemperature() {
  // Example: Read analog sensor and convert to temperature
  // Replace with your actual sensor code (DHT22, DS18B20, etc.)
  
  // Simulated reading with some variance
  float baseTemp = 65.0;
  float variance = (random(0, 100) - 50) / 10.0;
  return baseTemp + variance;
  
  /* Real sensor example (analog):
  int raw = analogRead(TEMP_SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float temperature = (voltage - 0.5) * 100.0;  // TMP36 formula
  return temperature;
  */
}

float readVibration() {
  // Example: Read vibration sensor
  // Replace with your actual sensor code (ADXL345, MPU6050, etc.)
  
  // Simulated reading
  float baseVibe = 0.010;
  float variance = (random(0, 100) - 50) / 10000.0;
  return max(0.0f, baseVibe + variance);
  
  /* Real sensor example (analog):
  int raw = analogRead(VIBE_SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float gForce = voltage / 0.33;  // Example conversion
  return gForce;
  */
}

int readRPM() {
  // Example: Read RPM from hall effect sensor or tachometer
  // Replace with your actual sensor code
  
  // Simulated reading
  int baseRPM = 3200;
  int variance = random(-100, 100);
  return max(0, baseRPM + variance);
  
  /* Real sensor example (pulse counting):
  static unsigned long lastTime = 0;
  static int pulseCount = 0;
  
  // Count pulses for 1 second
  unsigned long now = millis();
  if (now - lastTime >= 1000) {
    int rpm = pulseCount * 60;  // Convert pulses/sec to RPM
    pulseCount = 0;
    lastTime = now;
    return rpm;
  }
  return 0;
  */
}

// ===== DEEP SLEEP =====
void enterDeepSleep() {
  Serial.print("Entering deep sleep for ");
  Serial.print(SLEEP_SECONDS);
  Serial.println(" seconds...\n");
  
  delay(100);  // Allow serial output to complete
  
  // Configure wake-up timer
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  
  // Enter deep sleep
  esp_deep_sleep_start();
  
  // Execution never reaches here - ESP32 resets on wake
}
