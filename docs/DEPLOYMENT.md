# Deployment Guide

Complete guide for deploying the Industrial Monitoring Platform on Raspberry Pi 5.

## Prerequisites

### Hardware Requirements
- Raspberry Pi 5 (4GB RAM minimum, 8GB recommended)
- MicroSD card (32GB minimum, Class 10 or better)
- Stable power supply (5V 5A USB-C recommended)
- Network connectivity (Ethernet or Wi-Fi)
- Optional: Heat sink or active cooling

### Software Requirements
- Raspberry Pi OS Lite 64-bit (Debian 12 Bookworm)
- Docker Engine 20.10+
- Docker Compose v2.0+
- SSH access configured

## Initial Pi Setup

### 1. Install Raspberry Pi OS
```bash
# Use Raspberry Pi Imager or write image manually
# Enable SSH during imaging process
```

### 2. First Boot Configuration
```bash
# SSH into Pi
ssh pi@raspberrypi.local

# Update system
sudo apt update && sudo apt upgrade -y

# Set timezone
sudo timedatectl set-timezone America/New_York

# Set hostname (optional)
sudo hostnamectl set-hostname industrial-monitor
```

### 3. Install Docker
```bash
# Install Docker (if not already installed)
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add user to docker group
sudo usermod -aG docker $USER

# Logout and login for group changes
exit
# SSH back in

# Verify installation
docker --version
docker compose version
```

## Project Deployment

### 1. Create Project Directory
```bash
cd ~
mkdir -p industrial-monitor
cd industrial-monitor
```

### 2. Transfer Files
**Option A: Direct copy (if files are local)**
```bash
# From your development machine
scp -r industrial-monitor/* pi@raspberrypi.local:~/industrial-monitor/
```

**Option B: Git clone (when repo is ready)**
```bash
git clone <your-repo-url> ~/industrial-monitor
cd industrial-monitor
```

### 3. Configure Environment
```bash
# Copy template
cp .env.example .env

# Edit configuration
nano .env

# Generate secure passwords:
# - INFLUX_PASSWORD
# - INFLUX_TOKEN
# - GRAFANA_PASSWORD
```

### 4. Create Required Directories
```bash
# Ensure all data directories exist with proper permissions
mkdir -p mosquitto/{config,data,log}
mkdir -p influxdb/data
mkdir -p nodered/data
mkdir -p grafana/data

# Set appropriate permissions
chmod -R 755 mosquitto/
chmod -R 755 grafana/
```

### 5. Start Services
```bash
# Pull images (may take 10-15 minutes on first run)
docker compose pull

# Start all services
docker compose up -d

# Verify all containers are running
docker compose ps
```

### 6. Verify Deployment
```bash
# Check logs for errors
docker compose logs

# Test MQTT broker
docker exec industrial-mqtt mosquitto_sub -t "test/#" -v

# Access services
# Grafana: http://<pi-ip>:3000
# Node-RED: http://<pi-ip>:1880
# InfluxDB: http://<pi-ip>:8086
```

## Post-Deployment Configuration

### Mosquitto Configuration
Will be configured in Phase 2

### InfluxDB Setup
1. Access http://<pi-ip>:8086
2. Login with credentials from .env
3. Verify bucket `industrial_metrics` exists
4. Generate API token for Node-RED

### Node-RED Setup
1. Access http://<pi-ip>:1880
2. Install required nodes (Phase 4)
3. Import flow definitions

### Grafana Setup
1. Access http://<pi-ip>:3000
2. Login with credentials from .env
3. Add InfluxDB data source
4. Import dashboards

## Resource Management

### Monitor Resource Usage
```bash
# Container stats
docker stats

# System resources
htop

# Disk usage
df -h
du -sh industrial-monitor/*/data
```

### Configure Resource Limits
Edit `docker-compose.yml` to add:
```yaml
services:
  influxdb:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 2G
```

## Backup Strategy

### Manual Backup
```bash
# Stop services
docker compose down

# Backup data directories
tar -czf backup-$(date +%Y%m%d).tar.gz \
  influxdb/data \
  grafana/data \
  nodered/data \
  mosquitto/config

# Restart services
docker compose up -d
```

### Automated Backup (cron)
```bash
# Create backup script (scripts/backup.sh will be created in Phase 9)
crontab -e

# Add daily backup at 2 AM
0 2 * * * /home/pi/industrial-monitor/scripts/backup.sh
```

## Troubleshooting

### Services Won't Start
```bash
# Check Docker daemon
sudo systemctl status docker

# Check logs
docker compose logs [service_name]

# Check port conflicts
sudo netstat -tulpn | grep -E '(1883|1880|8086|3000)'
```

### Out of Memory
```bash
# Check memory
free -h

# Increase swap (if needed)
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile  # Set CONF_SWAPSIZE=2048
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### Disk Full
```bash
# Clean Docker images
docker system prune -a

# Check large directories
du -sh industrial-monitor/*/data

# Adjust InfluxDB retention policy
```

## Security Hardening

### Change Default Passwords
All passwords in `.env` must be changed from defaults

### Enable Firewall
```bash
sudo apt install ufw
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow ssh
sudo ufw allow from 192.168.1.0/24 to any port 3000  # Grafana
sudo ufw enable
```

### Disable SSH Password Auth
```bash
# Copy SSH key first!
ssh-copy-id pi@raspberrypi.local

# Then disable password auth
sudo nano /etc/ssh/sshd_config
# Set: PasswordAuthentication no
sudo systemctl restart ssh
```

## Performance Tuning

### InfluxDB Optimization
```bash
# Edit influxdb config for write performance
# (specific settings in Phase 3)
```

### Docker Performance
```bash
# Enable Docker log rotation
sudo nano /etc/docker/daemon.json
# Add:
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  }
}
sudo systemctl restart docker
```

## Maintenance

### Update Services
```bash
# Pull latest images
docker compose pull

# Recreate containers
docker compose up -d

# Clean old images
docker image prune
```

### Log Rotation
```bash
# Configure logrotate for application logs
# (specific configuration in Phase 9)
```

## Next Steps

After successful deployment:
1. Proceed to Phase 2: Configure Mosquitto
2. Set up InfluxDB retention policies (Phase 3)
3. Build Node-RED flows (Phase 4)
4. Create Grafana dashboards (Phase 5)
