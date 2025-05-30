# Server Setup Guide - MQTT & Database Infrastructure

This guide covers setting up the backend infrastructure for WiFi data logging using Docker Compose with Mosquitto MQTT broker and MariaDB database.

## 🏗️ **Infrastructure Overview**

```
ESP32 → WiFi → MQTT Broker → Database → Web Interface
                    ↓
              Real-time Logging
```

### Components
- **Mosquitto MQTT Broker**: Message routing and authentication
- **MariaDB Database**: Session and event data storage
- **phpMyAdmin**: Database management interface
- **Docker Compose**: Container orchestration

---

## 📋 **Prerequisites**

### Server Requirements
- **Debian 12** (or similar Linux distribution)
- **Docker & Docker Compose** installed
- **SSH access** to server
- **Local network connectivity**

### Install Docker (if not already installed)
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Install Docker Compose
sudo apt install docker-compose-plugin

# Add user to docker group
sudo usermod -aG docker $USER
# Log out and back in for group changes to take effect
```

---

## 🚀 **Installation Steps**

### Step 1: Create Project Directory
```bash
sudo mkdir -p /opt/stacks/mqtt
cd /opt/stacks/mqtt
```

### Step 2: Create Docker Compose Configuration

Create `compose.yaml`:
```yaml
services:
  mosquitto:
    image: eclipse-mosquitto
    container_name: mosquitto
    volumes:
      - ./config:/mosquitto/config
      - ./data:/mosquitto/data
      - ./log:/mosquitto/log
    ports:
      - 1883:1883
      - 9001:9001
    stdin_open: true 
    tty: true
    networks:
      - mqtt_network

  mariadb:
    image: mariadb:latest
    container_name: mariadb_mqtt
    environment:
      MYSQL_ROOT_PASSWORD: aes_root_2024
      MYSQL_DATABASE: rehab_exoskeleton
      MYSQL_USER: jay
      MYSQL_PASSWORD: aes
    volumes:
      - ./mariadb_data:/var/lib/mysql
      - ./mariadb_config:/etc/mysql/conf.d
      - ./sql_init:/docker-entrypoint-initdb.d
    ports:
      - 3306:3306
    restart: unless-stopped
    networks:
      - mqtt_network

  phpmyadmin:
    image: phpmyadmin/phpmyadmin
    container_name: phpmyadmin_mqtt
    environment:
      PMA_HOST: mariadb
      PMA_USER: jay
      PMA_PASSWORD: aes
      MYSQL_ROOT_PASSWORD: aes_root_2024
    ports:
      - 8080:80
    depends_on:
      - mariadb
    networks:
      - mqtt_network

networks:
  mqtt_network:
    driver: bridge

volumes:
  mariadb_data:
  mariadb_config:
  sql_init:
```

### Step 3: Configure Mosquitto MQTT Broker

Create directory and config:
```bash
sudo mkdir -p config
```

Create `config/mosquitto.conf`:
```
listener 1883
listener 9001
protocol websockets
persistence true
persistence_file mosquitto.db
persistence_location /mosquitto/data/

#Authentication
allow_anonymous false
password_file /mosquitto/config/pwfile
```

Create MQTT user credentials:
```bash
# Create password file
sudo docker run --rm -v $(pwd)/config:/mosquitto/config eclipse-mosquitto mosquitto_passwd -c /mosquitto/config/pwfile jay

# Enter password when prompted: aes
```

Set permissions:
```bash
sudo chmod 0700 config/pwfile
```

### Step 4: Create Database Schema

Create `sql_init/init_rehab_db.sql`:
```sql
-- Rehabilitation Exoskeleton Database Schema
USE rehab_exoskeleton;

-- Sessions table
CREATE TABLE IF NOT EXISTS sessions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    session_id VARCHAR(50) UNIQUE NOT NULL,
    device_id VARCHAR(50) NOT NULL,
    user_id VARCHAR(50),
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP NULL,
    duration_seconds INT DEFAULT 0,
    session_type ENUM('sequential', 'simultaneous', 'both') NOT NULL,
    total_cycles INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_session_id (session_id),
    INDEX idx_device_id (device_id),
    INDEX idx_start_time (start_time)
);

-- Events table for detailed logging
CREATE TABLE IF NOT EXISTS events (
    id INT AUTO_INCREMENT PRIMARY KEY,
    session_id VARCHAR(50),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    event_type ENUM('session_start', 'session_end', 'movement_command', 'movement_complete', 'ble_connect', 'ble_disconnect', 'system_error') NOT NULL,
    command VARCHAR(10),
    response_time_ms INT,
    servo_data JSON,
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (session_id) REFERENCES sessions(session_id) ON DELETE CASCADE,
    INDEX idx_session_id (session_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_event_type (event_type)
);

-- System status table
CREATE TABLE IF NOT EXISTS system_status (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_id VARCHAR(50) NOT NULL,
    status ENUM('online', 'offline', 'error') NOT NULL,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(45),
    firmware_version VARCHAR(20),
    uptime_seconds INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_device_id (device_id),
    INDEX idx_last_seen (last_seen)
);

-- Insert initial system status
INSERT INTO system_status (device_id, status, firmware_version) 
VALUES ('ESP32_001', 'offline', '1.0.0') 
ON DUPLICATE KEY UPDATE last_seen = CURRENT_TIMESTAMP;
```

---

## 🚀 **Deployment**

### Start the Stack
```bash
cd /opt/stacks/mqtt
sudo docker-compose up -d
```

### Verify Deployment
```bash
# Check container status
sudo docker-compose ps

# Check logs
sudo docker-compose logs
```

---

## ✅ **Verification & Testing**

### Test MQTT Broker
```bash
# Install MQTT clients (if not already installed)
sudo apt install mosquitto-clients

# Test publish
mosquitto_pub -h localhost -p 1883 -u jay -P aes -t "test/topic" -m "Hello MQTT"

# Test subscribe (in another terminal)
mosquitto_sub -h localhost -p 1883 -u jay -P aes -t "test/topic"
```

### Test Database Connection
```bash
# Connect to MariaDB
sudo docker exec -it mariadb_mqtt mariadb -u jay -paes rehab_exoskeleton

# Verify tables (if init script didn't run, create manually)
SHOW TABLES;
```

### Manual Table Creation (if needed)
If tables don't exist, run the SQL from `sql_init/init_rehab_db.sql` manually in the MariaDB prompt.

---

## 🌐 **Access Points**

| Service | URL/Address | Credentials |
|---------|-------------|-------------|
| **MQTT Broker** | `localhost:1883` | jay / aes |
| **MQTT WebSocket** | `localhost:9001` | jay / aes |
| **MariaDB** | `localhost:3306` | jay / aes |
| **phpMyAdmin** | `http://localhost:8080` | jay / aes |

---

## 🔧 **Management Commands**

### Container Management
```bash
# Start services
sudo docker-compose up -d

# Stop services
sudo docker-compose down

# Restart services
sudo docker-compose restart

# View logs
sudo docker-compose logs -f

# Update containers
sudo docker-compose pull
sudo docker-compose up -d
```

### Database Management
```bash
# Connect to database
sudo docker exec -it mariadb_mqtt mariadb -u jay -paes rehab_exoskeleton

# Backup database
sudo docker exec mariadb_mqtt mysqldump -u jay -paes rehab_exoskeleton > backup.sql

# Restore database
sudo docker exec -i mariadb_mqtt mysql -u jay -paes rehab_exoskeleton < backup.sql
```

---

## 🛡️ **Security Considerations**

### Current Setup (Development)
- **MQTT Authentication**: Username/password (jay/aes)
- **Database Access**: Local network only
- **Web Interface**: No HTTPS (local development)

### Production Recommendations
- **MQTT TLS**: Enable SSL/TLS encryption
- **Database SSL**: Secure database connections
- **Firewall**: Restrict port access
- **Strong Passwords**: Change default credentials
- **VPN Access**: Secure remote access

---

## 🔍 **Troubleshooting**

### Common Issues

**Containers won't start:**
```bash
# Check logs for errors
sudo docker-compose logs

# Check disk space
df -h

# Check permissions
sudo chown -R 1001:1001 mariadb_data
```

**MQTT connection refused:**
```bash
# Check if mosquitto is running
sudo docker-compose ps

# Test without authentication
mosquitto_pub -h localhost -p 1883 -t "test" -m "test"
```

**Database connection failed:**
```bash
# Check MariaDB logs
sudo docker-compose logs mariadb

# Verify database exists
sudo docker exec -it mariadb_mqtt mariadb -u root -paes_root_2024 -e "SHOW DATABASES;"
```

### Log Locations
- **Mosquitto logs**: `./log/mosquitto.log`
- **MariaDB logs**: `sudo docker-compose logs mariadb`
- **Container logs**: `sudo docker-compose logs [service_name]`

---

## 📊 **Next Steps**

After successful setup:
1. **ESP32 Integration**: Configure ESP32 to publish MQTT data
2. **Web Interface**: Create web dashboard for data visualization
3. **Mobile App Integration**: Add data logging to React Native app
4. **Data Analysis**: Implement session tracking and progress monitoring

---

## 📝 **Configuration Summary**

**MQTT Topics Structure:**
```
rehab_exo/
├── device_001/
│   ├── session/start
│   ├── session/end
│   ├── movement/command
│   ├── movement/complete
│   ├── system/status
│   └── connection/ble
```

**Database Schema:**
- **sessions**: Therapy session tracking
- **events**: Detailed command/response logging
- **system_status**: Device health monitoring

**Network Configuration:**
- **Internal Network**: `mqtt_network` (Docker bridge)
- **External Ports**: 1883 (MQTT), 9001 (WebSocket), 3306 (MariaDB), 8080 (phpMyAdmin)
