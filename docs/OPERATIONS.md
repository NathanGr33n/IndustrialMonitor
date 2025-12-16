# Operations Manual

Day-to-day operations guide for the Industrial Monitoring Platform.

## Daily Operations

### Health Checks
```bash
# Check all services are running
docker compose ps

# View recent logs
docker compose logs --tail=100

# Check resource usage
docker stats --no-stream
```

### Monitoring Key Metrics
- Service uptime
- Message throughput (MQTT)
- Database write rate
- Disk usage
- CPU/Memory utilization

## Common Tasks

### Restart Services
```bash
# Restart all services
docker compose restart

# Restart single service
docker compose restart mosquitto
docker compose restart influxdb
docker compose restart nodered
docker compose restart grafana
```

### View Logs
```bash
# Follow logs for all services
docker compose logs -f

# View logs for specific service
docker compose logs -f mosquitto

# View last 100 lines
docker compose logs --tail=100 influxdb
```

### Test MQTT Connectivity
```bash
# Subscribe to all factory topics
docker exec industrial-mqtt mosquitto_sub -t "factory/#" -v

# Publish test message
docker exec industrial-mqtt mosquitto_pub \
  -t "factory/test_machine" \
  -m '{"machine_id":"test_machine","temperature":25.0,"vibration":0.001,"rpm":1000}'
```

### Query InfluxDB
```bash
# Enter InfluxDB container
docker exec -it industrial-influxdb influx

# Or use CLI directly
docker exec industrial-influxdb influx query \
  'from(bucket: "industrial_metrics") 
   |> range(start: -1h) 
   |> filter(fn: (r) => r._measurement == "machine_metrics")'
```

## Adding New Sensors

### 1. Configure Sensor Node
- Set unique machine_id
- Configure MQTT broker address
- Set topic: `factory/{machine_id}`
- Use standard JSON payload format

### 2. Verify Data Flow
```bash
# Monitor MQTT for new device
docker exec industrial-mqtt mosquitto_sub -t "factory/new_machine_id" -v

# Check InfluxDB for data
# (via Grafana or CLI)
```

### 3. Create Dashboard
- Open Grafana
- Create new dashboard panel
- Filter by machine tag
- Save dashboard

## Backup & Restore

### Manual Backup
```bash
# Stop services (optional but recommended)
docker compose down

# Create backup
sudo tar -czf /backup/industrial-monitor-$(date +%Y%m%d-%H%M%S).tar.gz \
  -C ~/industrial-monitor \
  influxdb/data \
  grafana/data \
  nodered/data \
  mosquitto/config \
  .env

# Restart services
docker compose up -d
```

### Restore from Backup
```bash
# Stop services
docker compose down

# Extract backup
sudo tar -xzf /backup/industrial-monitor-YYYYMMDD-HHMMSS.tar.gz \
  -C ~/industrial-monitor

# Fix permissions
sudo chown -R $USER:$USER ~/industrial-monitor

# Start services
docker compose up -d
```

## Troubleshooting

### Service Not Responding

**Symptoms**: Container exists but service unreachable

**Diagnosis**:
```bash
docker compose ps  # Check status
docker compose logs [service_name]  # Check errors
docker inspect [container_name]  # Check network
```

**Resolution**:
```bash
docker compose restart [service_name]
# If that fails:
docker compose down
docker compose up -d
```

### High Memory Usage

**Symptoms**: System sluggish, OOM errors

**Diagnosis**:
```bash
free -h
docker stats --no-stream
```

**Resolution**:
```bash
# Restart resource-heavy services
docker compose restart influxdb

# Adjust retention policy (InfluxDB)
# Reduce Grafana refresh rates
# Add memory limits in docker-compose.yml
```

### Disk Full

**Symptoms**: Write errors, service crashes

**Diagnosis**:
```bash
df -h
du -sh ~/industrial-monitor/*/data
```

**Resolution**:
```bash
# Clean Docker system
docker system prune -a

# Adjust InfluxDB retention
# (reduce from 30d to 7d if needed)

# Archive old data
# Delete old backups
```

### MQTT Messages Not Reaching InfluxDB

**Symptoms**: MQTT shows messages, InfluxDB empty

**Diagnosis**:
```bash
# Check Node-RED logs
docker compose logs nodered

# Verify Node-RED flow active
# Check InfluxDB connectivity from Node-RED
```

**Resolution**:
- Verify Node-RED flow configuration
- Check InfluxDB credentials
- Restart Node-RED service

### No Data in Grafana

**Symptoms**: Dashboards show "No Data"

**Diagnosis**:
- Check InfluxDB has data
- Verify Grafana datasource configuration
- Check time range in dashboard

**Resolution**:
- Re-configure InfluxDB datasource
- Verify bucket name matches
- Check API token validity

## Performance Optimization

### InfluxDB Write Performance
- Batch writes in Node-RED
- Use appropriate retention policies
- Monitor shard size

### Grafana Dashboard Performance
- Increase refresh intervals
- Use query caching
- Limit time ranges on heavy queries

### Node-RED Optimization
- Avoid infinite loops
- Use rate limiting nodes
- Enable persistent context (if needed)

## Security Operations

### Rotate Passwords
```bash
# Update .env file
nano ~/industrial-monitor/.env

# Recreate services with new credentials
docker compose down
docker compose up -d
```

### Enable MQTT Authentication
(Will be configured in Phase 2)

### Review Access Logs
```bash
# Grafana access logs
docker compose logs grafana | grep "HTTP request"

# MQTT connection logs
docker compose logs mosquitto | grep "New connection"
```

## Maintenance Windows

### Update Services
```bash
# Pull latest images
docker compose pull

# Recreate containers with new images
docker compose up -d

# Remove old images
docker image prune -a
```

### Database Maintenance
```bash
# Compact InfluxDB (if needed)
docker exec industrial-influxdb influx task create \
  --name compact \
  --cron "0 2 * * *" \
  --file /path/to/compact.flux
```

## Monitoring Checklist

**Daily**:
- [ ] All services running
- [ ] No error logs
- [ ] Data flowing to Grafana

**Weekly**:
- [ ] Disk usage < 80%
- [ ] Backup successful
- [ ] Review alert history

**Monthly**:
- [ ] Update Docker images
- [ ] Review retention policies
- [ ] Performance review
- [ ] Security audit

## Emergency Procedures

### Complete System Failure
1. Check power and network
2. Reboot Raspberry Pi
3. Verify Docker daemon running
4. Start services: `docker compose up -d`
5. Verify data integrity

### Data Loss Event
1. Stop services immediately
2. Assess extent of loss
3. Restore from most recent backup
4. Document incident
5. Review backup strategy

## Contact Information

- System Administrator: [Your Name]
- On-call Support: [Contact Info]
- Escalation Path: [Escalation Process]

## Change Management

All configuration changes should:
1. Be documented in git commit messages
2. Be tested in development first
3. Have rollback plan ready
4. Be scheduled during maintenance windows
