# LoRa Sensor Node - ESP32

## Overview
Autonomous sensor node that reads telemetry and transmits via LoRa. Uses deep sleep between transmissions for battery-powered operation.

## Hardware Requirements
- **ESP32 Dev Board**
- **SX1276 or SX1278 LoRa Module**
- **Antenna** (915MHz for US, 868MHz for EU)
- **Sensors** (temperature, vibration, RPM - optional for testing)
- Power supply (USB or battery)

## Wiring Diagram

### LoRa Module
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

### Sensors (Example)
```
Sensor       →  ESP32
--------------------------
Temperature  →  GPIO 34 (analog)
Vibration    →  GPIO 35 (analog)
```

## Required Libraries
- **LoRa** by Sandeep Mistry
- **ArduinoJson** by Benoit Blanchon (v6.x)

## Configuration

### Machine Identity
Each node must have a unique ID:
```cpp
const char* MACHINE_ID = "lathe_01";  // Change for each node
```

Examples: `lathe_01`, `mill_02`, `press_03`

### Transmission Interval
```cpp
#define SLEEP_SECONDS 30  // Transmit every 30 seconds
```

Adjust based on application needs and battery life.

### LoRa Settings
```cpp
#define LORA_FREQUENCY 915E6  // US: 915MHz, EU: 868MHz
```

⚠️ **MUST match gateway configuration exactly**

## Operation

### Power-On Sequence
1. Wake from deep sleep (or cold boot)
2. Initialize LoRa radio
3. Read sensors
4. Transmit data packet
5. Enter deep sleep
6. Repeat after sleep interval

### Serial Monitor Output
```
=== LoRa Sensor Node ===
Machine ID: lathe_01
Boot count: 1
Initializing LoRa...
LoRa initialized
Reading sensors...
Temperature: 67.3 °C
Vibration: 0.012 g
RPM: 3200
Payload: {"id":"lathe_01","t":67.3,"v":0.012,"r":3200}
Payload size: 50 bytes
Transmitting...
✓ Transmission successful
Entering deep sleep for 30 seconds...
```

## Payload Format

### Transmitted (Compact JSON)
```json
{"id":"lathe_01","t":67.3,"v":0.012,"r":3200}
```

Field mapping:
- `id` → machine_id
- `t` → temperature (°C)
- `v` → vibration (g-force)
- `r` → rpm

Compact format minimizes airtime and power consumption.

## Adding Real Sensors

### Temperature (DHT22)
```cpp
#include <DHT.h>
DHT dht(TEMP_SENSOR_PIN, DHT22);

float readTemperature() {
  return dht.readTemperature();
}
```

### Vibration (ADXL345)
```cpp
#include <Adafruit_ADXL345_U.h>
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified();

float readVibration() {
  sensors_event_t event;
  accel.getEvent(&event);
  return sqrt(pow(event.acceleration.x, 2) + 
              pow(event.acceleration.y, 2) + 
              pow(event.acceleration.z, 2));
}
```

## Power Consumption

### Active Mode (~150mA)
- Radio transmission: ~120mA
- CPU: ~30mA
- Duration: ~2 seconds

### Deep Sleep (~10µA)
- RTC active
- RAM retained
- Duration: 30 seconds (configurable)

### Battery Life Estimation
With 2000mAh battery:
- 30s interval: ~3-6 months
- 5min interval: ~1-2 years

## Troubleshooting

### LoRa init failed
- Check wiring (especially CS, RST, DIO0)
- Verify 3.3V power (NOT 5V)
- Confirm correct frequency for module

### No data at gateway
- Verify frequency matches gateway
- Check sync word matches (0x12)
- Confirm spreading factor matches
- Check antenna connection
- Monitor serial output for transmission confirmation

### Sensor readings incorrect
- Check sensor wiring
- Verify voltage levels
- Calibrate sensor values
- Review sensor datasheet

### Short battery life
- Increase `SLEEP_SECONDS`
- Reduce TX power if range allows
- Check for current leaks
- Use quality battery

## Testing Without Sensors

The code includes simulated sensor readings for testing. You can:
1. Upload code as-is
2. Verify LoRa transmission works
3. Add real sensors incrementally

## Multiple Nodes

To deploy multiple nodes:
1. Change `MACHINE_ID` for each (e.g., `lathe_01`, `lathe_02`)
2. Upload to each ESP32
3. All nodes use same LoRa settings
4. Gateway handles all nodes automatically

## Range Testing

Expected range (varies with environment):
- **Line of sight**: 2-5 km
- **Urban**: 500m - 2km  
- **Indoor**: 100-500m

Factors affecting range:
- Antenna quality
- TX power
- Spreading factor
- Obstacles (walls, metal)
- Interference

## Next Steps
After testing nodes:
1. Verify gateway receives data
2. Check MQTT messages on server
3. Configure Node-RED for data processing
4. Set up Grafana dashboards
