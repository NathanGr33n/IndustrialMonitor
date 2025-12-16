# LoRa Integration Guide

Complete guide for integrating LoRa-based sensor nodes with the Industrial Monitoring Platform.

## Overview

LoRa enables long-range, low-power wireless communication for sensor nodes that cannot use Wi-Fi. This guide covers the gateway-based architecture where LoRa nodes transmit to a gateway, which forwards data to the MQTT broker.

## Architecture

```
┌─────────────────┐
│ LoRa Sensor     │ (Battery powered, remote)
│ Node (ESP32)    │
└────────┬────────┘
         │ LoRa Radio
         │ (433/868/915 MHz)
         ▼
┌─────────────────┐
│ LoRa Gateway    │ (Mains powered, central)
│ (ESP32/RPi)     │
└────────┬────────┘
         │ Wi-Fi/Ethernet
         │ MQTT Publish
         ▼
┌─────────────────┐
│ MQTT Broker     │ (Mosquitto)
│ (Raspberry Pi)  │
└─────────────────┘
```

**Key Points**:
- LoRa nodes DO NOT connect directly to the server
- Gateway bridges LoRa ↔ MQTT
- Multiple nodes can share one gateway
- Gateway translates binary/compact payloads to standard JSON

## Hardware Requirements

### LoRa Sensor Node
- **MCU**: ESP32 (or similar)
- **LoRa Module**: SX1276 / SX1278 based
- **Frequency**: 
  - US: 915 MHz (US915)
  - EU: 868 MHz (EU868)
  - Asia: 433 MHz
- **Antenna**: Matched to frequency band
- **Power**: Battery (18650, LiPo) + solar (optional)
- **Sensors**: Temperature, vibration, etc.

### LoRa Gateway
- **Option A**: ESP32 + LoRa module + Wi-Fi
- **Option B**: Raspberry Pi + LoRa HAT (SX1301/1302)
- **Power**: Mains powered (5V USB)
- **Network**: Wi-Fi or Ethernet

## LoRa Configuration

### Radio Parameters
```cpp
// Must match between nodes and gateway
#define LORA_FREQUENCY    915E6  // 915 MHz for US
#define LORA_BANDWIDTH    125E3  // 125 kHz
#define LORA_SPREADING    7      // SF7 (fast, short range)
#define LORA_CODING_RATE  5      // 4/5
#define LORA_TX_POWER     20     // 20 dBm (max)
```

**Trade-offs**:
- Higher SF = longer range, slower speed, more power
- Lower SF = shorter range, faster speed, less power
- Typical: SF7-SF9 for industrial environments

### Range Expectations
- **SF7**: ~2 km line-of-sight, ~500m urban
- **SF9**: ~5 km line-of-sight, ~1-2km urban
- **SF12**: ~15 km line-of-sight, ~3-5km urban

## LoRa Node Implementation

### Minimal ESP32 LoRa Node Example

```cpp
#include <LoRa.h>
#include <ArduinoJson.h>

// LoRa Pins (adjust for your board)
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS   18
#define LORA_RST  14
#define LORA_DIO0 26

const char* MACHINE_ID = "lathe_01";
const unsigned long SEND_INTERVAL = 5000; // 5 seconds

void setup() {
  Serial.begin(115200);
  
  // Initialize LoRa
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(20);
  
  Serial.println("LoRa node ready");
}

void loop() {
  // Read sensors
  float temperature = readTemperature();
  float vibration = readVibration();
  int rpm = readRPM();
  
  // Build compact JSON payload
  StaticJsonDocument<128> doc;
  doc["id"] = MACHINE_ID;
  doc["t"] = temperature;
  doc["v"] = vibration;
  doc["r"] = rpm;
  
  String payload;
  serializeJson(doc, payload);
  
  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
  
  Serial.println("Sent: " + payload);
  
  // Deep sleep to save power (optional)
  delay(SEND_INTERVAL);
}

float readTemperature() {
  // TODO: Read from sensor
  return 67.3;
}

float readVibration() {
  // TODO: Read from accelerometer
  return 0.012;
}

int readRPM() {
  // TODO: Read from encoder/hall sensor
  return 3200;
}
```

### Power Optimization

```cpp
#include <esp_sleep.h>

void enterDeepSleep(int seconds) {
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
  // Send data
  sendLoRaMessage();
  
  // Sleep for 5 minutes
  enterDeepSleep(300);
}
```

**Battery Life Calculation**:
- Send every 5 minutes
- 50mA active for 2 seconds
- 10µA sleep
- 2000mAh battery → ~6 months

## LoRa Gateway Implementation

### ESP32 Gateway (Simple)

```cpp
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "your_wifi";
const char* WIFI_PASSWORD = "your_password";
const char* MQTT_SERVER = "192.168.1.100";  // Your Pi IP
const int MQTT_PORT = 1883;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // Initialize LoRa
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(7);
  Serial.println("LoRa gateway ready");
  
  // Connect to MQTT
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();
  
  // Check for LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handleLoRaPacket();
  }
}

void handleLoRaPacket() {
  String payload = "";
  while (LoRa.available()) {
    payload += (char)LoRa.read();
  }
  
  int rssi = LoRa.packetRssi();
  Serial.println("Received: " + payload + " (RSSI: " + String(rssi) + ")");
  
  // Parse compact JSON
  StaticJsonDocument<128> compactDoc;
  deserializeJson(compactDoc, payload);
  
  // Transform to canonical format
  StaticJsonDocument<256> canonicalDoc;
  canonicalDoc["machine_id"] = compactDoc["id"].as<String>();
  canonicalDoc["timestamp"] = millis() / 1000;  // Or use NTP
  canonicalDoc["temperature"] = compactDoc["t"];
  canonicalDoc["vibration"] = compactDoc["v"];
  canonicalDoc["rpm"] = compactDoc["r"];
  canonicalDoc["rssi"] = rssi;  // Add signal quality
  
  String mqttPayload;
  serializeJson(canonicalDoc, mqttPayload);
  
  // Publish to MQTT
  String topic = "factory/" + String(compactDoc["id"].as<const char*>());
  mqtt.publish(topic.c_str(), mqttPayload.c_str());
  
  Serial.println("Published to MQTT: " + topic);
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect("lora-gateway")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqtt.state());
      delay(5000);
    }
  }
}
```

## Payload Design

### Compact LoRa Payload (Minimal airtime)

```json
{
  "id": "lathe_01",
  "t": 67.3,
  "v": 0.012,
  "r": 3200
}
```

**Size**: ~50 bytes
**Airtime (SF7, 125kHz)**: ~60ms

### Canonical MQTT Payload (Gateway output)

```json
{
  "machine_id": "lathe_01",
  "timestamp": 1710000000,
  "temperature": 67.3,
  "vibration": 0.012,
  "rpm": 3200,
  "rssi": -65
}
```

**Size**: ~150 bytes (MQTT can handle it)

## Deployment Strategy

### 1. Test LoRa Link
```cpp
// Node: Send test packets
// Gateway: Monitor reception and RSSI
```

### 2. Verify Gateway → MQTT
```bash
# Subscribe to topic
docker exec industrial-mqtt mosquitto_sub -t "factory/#" -v
```

### 3. Deploy Nodes
- Install at target locations
- Verify connectivity
- Monitor battery voltage

### 4. Monitor Signal Quality
- Track RSSI values
- Identify dead zones
- Adjust gateway placement

## Troubleshooting

### No LoRa Reception

**Check**:
- Frequency match (915 vs 868 MHz)
- Spreading factor match
- Antenna connected
- Line-of-sight obstructions

**Debug**:
```cpp
Serial.println("RSSI: " + String(LoRa.packetRssi()));
Serial.println("SNR: " + String(LoRa.packetSnr()));
```

### Gateway Not Publishing to MQTT

**Check**:
- WiFi connection
- MQTT broker IP address
- MQTT broker port (1883)
- Topic format

**Debug**:
```cpp
Serial.println("MQTT State: " + String(mqtt.state()));
// 0 = connected
// -1 = connection lost
// -2 = connect failed
```

### High Packet Loss

**Solutions**:
- Increase spreading factor (SF7 → SF9)
- Add gateway diversity (multiple gateways)
- Reduce transmission rate
- Check antenna quality

## Advanced Topics

### LoRaWAN Migration
For production systems, consider using LoRaWAN:
- Standardized protocol
- Network server infrastructure
- Over-the-air updates
- Better security

**Popular Options**:
- The Things Network (TTN)
- ChirpStack (self-hosted)
- Helium Network

### Security Considerations

**Current Implementation**: No encryption (development)

**Production Hardening**:
- Add AES encryption to payloads
- Device whitelisting at gateway
- Payload signing (HMAC)
- Secure boot on nodes

### Multiple Gateways

For larger deployments:
- Each gateway publishes to same MQTT topics
- InfluxDB deduplicates by timestamp
- Choose best RSSI if duplicates exist

## Performance Optimization

### Reduce Airtime
- Use binary payloads instead of JSON
- Compress data (delta encoding)
- Aggregate multiple readings

### Duty Cycle Compliance
EU regulations: 1% duty cycle on 868 MHz
- Calculate airtime per message
- Ensure < 36 seconds/hour transmission

### Battery Management
- Use deep sleep between transmissions
- Monitor battery voltage
- Send low-battery alerts
- Implement adaptive transmission rates

## Testing Checklist

- [ ] LoRa node transmits successfully
- [ ] Gateway receives packets (check RSSI)
- [ ] Gateway publishes to MQTT
- [ ] Data appears in InfluxDB
- [ ] Grafana displays LoRa node data
- [ ] Battery life meets requirements
- [ ] Range test completed
- [ ] Failover tested (node offline)

## Reference Resources

- **LoRa Library**: https://github.com/sandeepmistry/arduino-LoRa
- **LoRaWAN**: https://lora-alliance.org/
- **The Things Network**: https://www.thethingsnetwork.org/
- **ChirpStack**: https://www.chirpstack.io/

## Next Steps

1. Build LoRa node prototype
2. Build ESP32 gateway
3. Test communication range
4. Integrate with MQTT broker
5. Deploy and monitor
