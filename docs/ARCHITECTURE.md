# Architecture Guide

## System Overview

The Industrial Monitoring Platform follows a layered architecture designed for reliability, scalability, and maintainability in edge computing environments.

## Component Layers

### 1. Data Ingestion Layer
**Technology**: Eclipse Mosquitto (MQTT Broker)

**Responsibilities**:
- Accept connections from sensor nodes (Wi-Fi/LoRa gateway)
- Handle pub/sub messaging
- Provide message persistence and QoS
- Support TLS encryption (future)

**Topics**:
- `factory/{machine_id}` - Primary telemetry
- `factory/{machine_id}/status` - Device status (future)
- `factory/{machine_id}/command` - Control commands (future)

### 2. Data Processing Layer
**Technology**: Node-RED

**Responsibilities**:
- Validate incoming JSON payloads
- Normalize data formats
- Enrich with timestamps
- Route to storage
- Handle errors and retries

**Flow Structure**:
```
[MQTT Input] → [JSON Parser] → [Validation] → [Transform] → [InfluxDB Writer]
                                     ↓
                              [Error Handler]
```

### 3. Storage Layer
**Technology**: InfluxDB v2

**Responsibilities**:
- Store time-series metrics
- Handle high-throughput writes
- Support retention policies
- Provide query interface

**Schema Design**:
- Measurement-per-metric vs single measurement
- Tagging strategy for efficient queries
- Field data types

### 4. Visualization Layer
**Technology**: Grafana

**Responsibilities**:
- Real-time dashboards
- Historical trend analysis
- Alert management
- User access control

## Data Flow

```
Sensor → MQTT → Node-RED → InfluxDB → Grafana
  |        |         |          |         |
  |        |         |          |         └─ User Dashboard
  |        |         |          └─ Time-series storage
  |        |         └─ Validation & routing
  |        └─ Message buffering
  └─ Telemetry generation
```

## Network Architecture

All services communicate within a Docker bridge network (`industrial-net`):
- Internal DNS resolution
- Service isolation
- Port exposure only where needed

## Failure Modes & Recovery

### MQTT Broker Failure
- Clients reconnect automatically
- Message persistence enabled
- Docker restart policy

### InfluxDB Failure
- Node-RED buffering (limited)
- Data loss window: minutes
- Recovery: automatic restart

### Node-RED Failure
- MQTT messages queued at broker
- Recovery on restart
- No processing during downtime

### Grafana Failure
- No impact on data collection
- Dashboards unavailable
- Recovery: automatic restart

## Scalability Considerations

### Current Limitations
- Single InfluxDB instance
- No horizontal scaling
- 50-100 devices typical max

### Future Enhancements
- InfluxDB clustering
- Multiple Node-RED instances
- Load balancing
- Edge aggregation nodes

## Security Architecture

### Current State (Development)
- No MQTT authentication
- Default passwords
- No TLS/SSL
- Local network only

### Production Hardening
- MQTT username/password auth
- TLS certificates
- Grafana user roles
- Network segmentation
- Firewall rules

## Performance Characteristics

### Expected Throughput
- 50 devices @ 5s intervals = 10 msg/sec
- Average payload: 200 bytes
- Bandwidth: ~2 KB/s
- Storage: ~17 MB/day

### Latency Budget
- MQTT publish: <10ms
- Node-RED processing: <100ms
- InfluxDB write: <500ms
- Grafana refresh: 1-5s
- **Total**: <2s end-to-end

## Monitoring the Monitor

### Health Checks
- Docker container status
- Service HTTP endpoints
- MQTT connectivity
- InfluxDB write success rate

### Key Metrics
- Message throughput
- Processing latency
- Database write rate
- Disk usage
- CPU/Memory utilization

## Design Decisions

### Why MQTT?
- Industry standard for IoT
- Lightweight protocol
- Quality of Service levels
- Client libraries widely available

### Why Node-RED?
- Visual flow-based programming
- Low-code approach
- Rapid prototyping
- Extensive node library

### Why InfluxDB?
- Purpose-built for time-series
- Efficient storage compression
- Powerful query language (Flux)
- Retention policies built-in

### Why Grafana?
- Industry-leading visualization
- Extensive plugin ecosystem
- Built-in alerting
- Multi-datasource support

## Future Architecture Enhancements

1. **Edge Computing**: Pre-aggregation at gateway
2. **Machine Learning**: Anomaly detection
3. **Multi-tenancy**: Separate organizations
4. **Cloud Sync**: Optional cloud backup
5. **Real-time Control**: Bidirectional commands
