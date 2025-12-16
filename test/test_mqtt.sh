#!/bin/bash
# Test MQTT Broker Connectivity
# Simulates LoRa gateway publishing sensor data

echo "=== MQTT Test Script ==="
echo "Testing industrial monitoring MQTT broker"
echo ""

# Test 1: Publish test message
echo "Test 1: Publishing test message..."
docker exec industrial-mqtt mosquitto_pub -t "factory/test_node" \
  -m '{"machine_id":"test_node","timestamp":1234567890,"temperature":72.5,"vibration":0.015,"rpm":3150,"rssi":-50,"snr":8.5}'

if [ $? -eq 0 ]; then
  echo "✓ Publish successful"
else
  echo "✗ Publish failed"
  exit 1
fi

echo ""

# Test 2: Subscribe to factory topic
echo "Test 2: Subscribing to factory/# topic..."
echo "Publishing simulated sensor data..."
echo ""

# Start subscriber in background
docker exec industrial-mqtt mosquitto_sub -t "factory/#" -v > /tmp/mqtt_test.log 2>&1 &
SUB_PID=$!

sleep 2

# Publish multiple test messages
docker exec industrial-mqtt mosquitto_pub -t "factory/lathe_01" \
  -m '{"machine_id":"lathe_01","timestamp":1234567890,"temperature":67.3,"vibration":0.012,"rpm":3200,"rssi":-45,"snr":9.5}'

docker exec industrial-mqtt mosquitto_pub -t "factory/mill_02" \
  -m '{"machine_id":"mill_02","timestamp":1234567891,"temperature":71.8,"vibration":0.018,"rpm":2800,"rssi":-52,"snr":7.2}'

docker exec industrial-mqtt mosquitto_pub -t "factory/press_03" \
  -m '{"machine_id":"press_03","timestamp":1234567892,"temperature":69.1,"vibration":0.021,"rpm":1500,"rssi":-48,"snr":8.9}'

sleep 2

# Stop subscriber
kill $SUB_PID 2>/dev/null

echo "Received messages:"
cat /tmp/mqtt_test.log
echo ""

# Clean up
rm /tmp/mqtt_test.log

echo "=== Test Complete ==="
echo ""
echo "Next Steps:"
echo "1. Deploy ESP32 LoRa Gateway"
echo "2. Deploy ESP32 LoRa Sensor Nodes"
echo "3. Configure Node-RED for data processing"
echo "4. Set up Grafana dashboards"
