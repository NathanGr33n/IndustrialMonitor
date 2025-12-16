# Industrial Monitoring Platform

A self-hosted, industrial-grade monitoring system for Raspberry Pi 5 that collects real-time telemetry from distributed sensor nodes via Wi-Fi (MQTT) and LoRa, stores time-series data, and provides visualization and alerting capabilities.

## Overview

This platform mirrors real-world industrial monitoring stacks used in manufacturing, utilities, and infrastructure environments while remaining lightweight enough to run on edge hardware.

### Key Features

- **Real-time telemetry collection** from distributed sensors
- **Dual connectivity**: MQTT (Wi-Fi) and LoRa support
- **Time-series data storage** with configurable retention
- **Interactive dashboards** for metrics visualization
- **Automated alerting** on threshold breaches and device failures
- **Containerized deployment** for reliability and portability

## Architecture

```
┌─────────────────┐
│  Sensor Nodes   │ (Wi-Fi / LoRa)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ MQTT Broker     │ (Mosquitto)
│ Port: 1883      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Data Processor  │ (Node-RED)
│ Port: 1880      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Time-Series DB  │ (InfluxDB v2)
│ Port: 8086      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Visualization   │ (Grafana)
│ Port: 3000      │
└─────────────────┘
```

## Technology Stack

| Component | Technology | Purpose |
|-----------|-----------|---------|
| Message Broker | Mosquitto (MQTT) | Sensor data ingestion |
| Data Processor | Node-RED | Validation & routing |
| Database | InfluxDB v2 | Time-series storage |
| Visualization | Grafana | Dashboards & alerts |
| Runtime | Docker + Compose | Container orchestration |

## Quick Start

### Prerequisites

- Raspberry Pi 5 (4GB minimum, 8GB recommended)
- Raspberry Pi OS Lite 64-bit (or Debian-based distro)
- Docker Engine installed
- Docker Compose plugin installed

### Installation

1. **Clone the repository** (or copy files to Pi)
   ```bash
   cd ~/
   # If using git later: git clone <your-repo-url>
   cd industrial-monitor
   ```

2. **Configure environment**
   ```bash
   cp .env.example .env
   nano .env  # Update passwords and tokens
   ```

3. **Start services**
   ```bash
   docker compose up -d
   ```

4. **Verify services are running**
   ```bash
   docker compose ps
   ```

### Service Access

| Service | URL | Default Credentials |
|---------|-----|-------------------|
| Grafana | http://localhost:3000 | admin / (see .env) |
| Node-RED | http://localhost:1880 | None (configure later) |
| InfluxDB | http://localhost:8086 | admin / (see .env) |
| MQTT | mqtt://localhost:1883 | No auth (initially) |

## Data Model

### MQTT Topic Structure

```
factory/{machine_id}
```

Example: `factory/lathe_01`

### Payload Schema (JSON)

```json
{
  "machine_id": "lathe_01",
  "timestamp": 1710000000,
  "temperature": 67.3,
  "vibration": 0.012,
  "rpm": 3200,
  "uptime": 124920
}
```

### InfluxDB Schema

- **Measurement**: `machine_metrics`
- **Tags**: `machine` (machine_id)
- **Fields**: `temperature`, `vibration`, `rpm`, `uptime`
- **Timestamp**: Auto-generated or from payload

## Testing

### Publish Test Message

```bash
docker exec industrial-mqtt mosquitto_pub \
  -t "factory/lathe_01" \
  -m '{"machine_id":"lathe_01","temperature":67.3,"vibration":0.012,"rpm":3200}'
```

### Subscribe to All Topics

```bash
docker exec industrial-mqtt mosquitto_sub -t "factory/#" -v
```

## Management

### Start Services
```bash
docker compose up -d
```

### Stop Services
```bash
docker compose down
```

### View Logs
```bash
docker compose logs -f [service_name]
```

### Restart Single Service
```bash
docker compose restart [service_name]
```

## Performance Targets

- **Throughput**: ≥50 devices @ 5-second intervals
- **Latency**: <2 seconds end-to-end (publish → dashboard)
- **Reliability**: Auto-restart on failure, minimal data loss

## Security Considerations

- **Default deployment**: No MQTT authentication (development mode)
- **Production**: Enable MQTT auth, use strong passwords
- **Network**: Services isolated in Docker network
- **Access**: SSH key-only recommended for Pi

## Project Structure

```
industrial-monitor/
├── docker-compose.yml      # Service orchestration
├── .env                    # Secrets (not in git)
├── .env.example            # Configuration template
├── mosquitto/
│   └── config/            # MQTT broker config
├── nodered/
│   └── data/              # Flow definitions (export to git)
├── influxdb/
│   └── data/              # Database files (not in git)
├── grafana/
│   ├── data/              # Grafana state (not in git)
│   └── provisioning/      # Datasources & dashboards
├── docs/                  # Documentation
├── scripts/               # Utility scripts
├── examples/              # Sample code (nodes, clients)
└── test/                  # Testing utilities
```

## Next Steps

1. **Phase 2**: Configure Mosquitto MQTT broker
2. **Phase 3**: Set up InfluxDB database and retention
3. **Phase 4**: Build Node-RED data processing flows
4. **Phase 5**: Create Grafana dashboards
5. **Phase 6**: Implement alerting rules
6. **Phase 7**: Integrate LoRa gateway

See `docs/` for detailed guides.

## Troubleshooting

### Services won't start
```bash
docker compose logs [service_name]
```

### Port conflicts
Check if ports 1883, 1880, 8086, 3000 are already in use:
```bash
sudo netstat -tulpn | grep -E '(1883|1880|8086|3000)'
```

### Permission issues
```bash
sudo chown -R 1000:1000 ./grafana/data
sudo chown -R 1883:1883 ./mosquitto/data
```

## Documentation

- [Architecture Guide](docs/ARCHITECTURE.md)
- [Deployment Guide](docs/DEPLOYMENT.md)
- [Operations Manual](docs/OPERATIONS.md)
- [LoRa Integration](docs/LORA_GUIDE.md)

## License

MIT License - See LICENSE file for details

## Contributing

This is a learning/portfolio project. Fork and adapt for your needs!
