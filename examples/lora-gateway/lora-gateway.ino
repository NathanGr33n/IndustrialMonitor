/*
 * LoRa Gateway - Industrial Monitor
 * 
 * Receives LoRa packets from sensor nodes and forwards to MQTT broker
 * Translates compact LoRa payload into canonical JSON format
 * 
 * Hardware: ESP32 + SX1276/SX1278 LoRa module
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

#include <WiFi.h>
#include <PubSubClient.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// ===== CONFIGURATION =====
// WiFi Credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// MQTT Broker
const char* MQTT_BROKER = "192.168.1.100";  // Raspberry Pi IP
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "lora-gateway";
const char* MQTT_TOPIC_PREFIX = "factory/";

// LoRa Configuration
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

// ===== GLOBAL OBJECTS =====
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== LoRa Gateway Starting ===");
  
  // Initialize WiFi
  setupWiFi();
  
  // Initialize MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  
  // Initialize LoRa
  setupLoRa();
  
  Serial.println("=== Gateway Ready ===\n");
}

// ===== MAIN LOOP =====
void loop() {
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Check for LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleLoRaPacket(packetSize);
  }
  
  delay(10);
}

// ===== WIFI SETUP =====
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ===== LORA SETUP =====
void setupLoRa() {
  Serial.println("Initializing LoRa...");
  
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  LoRa.setSignalBandwidth(LORA_BANDWIDTH);
  LoRa.setSpreadingFactor(LORA_SPREADING);
  LoRa.setCodingRate4(LORA_CODING_RATE);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  
  Serial.println("LoRa initialized");
  Serial.print("Frequency: ");
  Serial.print(LORA_FREQUENCY / 1E6);
  Serial.println(" MHz");
}

// ===== MQTT RECONNECT =====
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT... ");
    
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

// ===== HANDLE LORA PACKET =====
void handleLoRaPacket(int packetSize) {
  Serial.println("\n--- LoRa Packet Received ---");
  
  // Read packet
  String payload = "";
  while (LoRa.available()) {
    payload += (char)LoRa.read();
  }
  
  // Get RSSI
  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();
  
  Serial.print("Payload: ");
  Serial.println(payload);
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm, SNR: ");
  Serial.print(snr);
  Serial.println(" dB");
  
  // Parse compact JSON from node
  StaticJsonDocument<256> rxDoc;
  DeserializationError error = deserializeJson(rxDoc, payload);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Extract fields (compact format from node)
  const char* machineId = rxDoc["id"];
  float temperature = rxDoc["t"];
  float vibration = rxDoc["v"];
  int rpm = rxDoc["r"];
  
  if (!machineId) {
    Serial.println("Missing machine ID!");
    return;
  }
  
  // Build canonical MQTT payload
  StaticJsonDocument<512> txDoc;
  txDoc["machine_id"] = machineId;
  txDoc["timestamp"] = millis() / 1000;  // Server will use this or generate own
  txDoc["temperature"] = temperature;
  txDoc["vibration"] = vibration;
  txDoc["rpm"] = rpm;
  txDoc["rssi"] = rssi;
  txDoc["snr"] = snr;
  
  // Serialize to JSON string
  char mqttPayload[512];
  serializeJson(txDoc, mqttPayload);
  
  // Publish to MQTT
  char topic[64];
  snprintf(topic, sizeof(topic), "%s%s", MQTT_TOPIC_PREFIX, machineId);
  
  Serial.print("Publishing to: ");
  Serial.println(topic);
  Serial.print("MQTT Payload: ");
  Serial.println(mqttPayload);
  
  if (mqttClient.publish(topic, mqttPayload)) {
    Serial.println("✓ Published to MQTT");
  } else {
    Serial.println("✗ MQTT publish failed");
  }
  
  Serial.println("----------------------------\n");
}
