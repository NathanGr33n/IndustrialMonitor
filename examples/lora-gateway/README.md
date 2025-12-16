# LoRa Gateway - ESP32

## Overview
Receives LoRa packets from sensor nodes and forwards them to the MQTT broker, translating compact LoRa payloads into canonical JSON format for the monitoring platform.

## Hardware Requirements
- **ESP32 Dev Board**
- **SX1276 or SX1278 LoRa Module**
- **Antenna** (915MHz for US, 868MHz for EU)
- Jumper wires

## Wiring Diagram
```
LoRa Module  →  ESP32
--------------------------
VCC          →  3.3V
GND          →  GND
SCK          →  GPIO 5
MISO         →  GPIO 19
MOSI         →  GPIO 27
CS (NSS)     →  GPIO 18
RST          →  GPIO 14
DIO0 (IRQ)   →  GPIO 26
```

## Required Libraries
Install via Arduino IDE Library Manager or PlatformIO:
- **LoRa** by Sandeep Mistry
- **PubSubClient** by Nick O'Leary
- **ArduinoJson** by Benoit Blanchon (v6.x)

## Configuration

Edit `lora-gateway.ino` and update:

### WiFi Settings
```cpp
const char* WIFI_SSID = "YourNetworkName";
const char* WIFI_PASSWORD = "YourPassword";
```

### MQTT Broker
```cpp
const char* MQTT_BROKER = "192.168.1.100";  // Your Raspberry Pi IP
```

### LoRa Frequency
```cpp
#define LORA_FREQUENCY 915E6  // US: 915MHz, EU: 868MHz
```

## Upload Process

### Using Arduino IDE
1. Open `lora-gateway.ino`
2. Select board: **ESP32 Dev Module**
3. Select correct port
4. Click Upload

### Using PlatformIO
```bash
pio run -t upload
```

## Operation

### Startup Sequence
1. Connects to WiFi
2. Connects to MQTT broker
3. Initializes LoRa radio
4. Enters receive mode

### Serial Monitor Output
```
=== LoRa Gateway Starting ===
Connecting to WiFi: MyNetwork
WiFi connected
IP: 192.168.1.150
Initializing LoRa...
LoRa initialized
Frequency: 915.0 MHz
=== Gateway Ready ===

--- LoRa Packet Received ---
Payload: {"id":"lathe_01","t":67.3,"v":0.012,"r":3200}
RSSI: -45 dBm, SNR: 9.5 dB
Publishing to: factory/lathe_01
MQTT Payload: {"machine_id":"lathe_01","timestamp":12345,"temperature":67.3,"vibration":0.012,"rpm":3200,"rssi":-45,"snr":9.5}
✓ Published to MQTT
----------------------------
```

## Data Flow
```
LoRa Node → [Compact JSON] → Gateway → [Canonical JSON] → MQTT → Server
```

### Input (from node)
```json
{"id":"lathe_01","t":67.3,"v":0.012,"r":3200}
```

### Output (to MQTT)
```json
{
  "machine_id": "lathe_01",
  "timestamp": 12345,
  "temperature": 67.3,
  "vibration": 0.012,
  "rpm": 3200,
  "rssi": -45,
  "snr": 9.5
}
```

## Troubleshooting

### WiFi won't connect
- Verify SSID and password
- Check signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

### LoRa init failed
- Check wiring
- Verify voltage (3.3V, NOT 5V)
- Confirm correct frequency for your module

### MQTT connection fails
- Verify Raspberry Pi IP address
- Check that Mosquitto is running: `docker ps`
- Test connectivity: `ping 192.168.1.100`

### No packets received
- Verify LoRa frequency matches node
- Check spreading factor matches
- Ensure sync word matches (0x12)
- Check antenna connection

## Testing

### Monitor MQTT messages on server
```bash
docker exec industrial-mqtt mosquitto_sub -t "factory/#" -v
```

### Expected output
```
factory/lathe_01 {"machine_id":"lathe_01","timestamp":12345,...}
```

## Performance
- **Range**: 2-5 km line-of-sight (varies with environment)
- **Latency**: < 500ms LoRa → MQTT
- **Throughput**: ~50 messages/second (depends on LoRa settings)

## Security Notes
⚠️ **Phase 2**: Anonymous MQTT access (development only)  
🔒 **Phase 3**: Add MQTT authentication and TLS
