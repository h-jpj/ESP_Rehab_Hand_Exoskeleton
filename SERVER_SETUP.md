# Server Setup Guide - MQTT & Database Infrastructure with Advanced Analytics

This guide covers setting up the backend infrastructure for WiFi data logging using Docker Compose with Mosquitto MQTT broker, MariaDB database, and enhanced analytics capabilities.

## 📋 **Quick Reference**
**🔗 [DATABASE_QUERIES.md](DATABASE_QUERIES.md)** - Complete database schemas, Grafana queries, and MQTT topics reference. **Start here for all queries and configurations!**

## 🔒 **Security Configuration**

**⚠️ IMPORTANT: Configure your credentials before deployment**

### Environment Setup

1. **Copy environment template:**
   ```bash
   cp .env.example .env
   ```

2. **Edit with your actual credentials:**
   ```bash
   nano .env
   ```

3. **Update these critical settings:**
   ```bash
   # WiFi Configuration
   WIFI_SSID=your_actual_wifi_name
   WIFI_PASSWORD=your_secure_wifi_password

   # Server Configuration
   SERVER_IP=your_server_ip_address

   # Database Configuration
   DB_ROOT_PASSWORD=your_secure_root_password
   DB_USER=your_db_username
   DB_PASSWORD=your_secure_db_password

   # MQTT Configuration
   MQTT_USER=your_mqtt_username
   MQTT_PASSWORD=your_secure_mqtt_password
   ```

4. **Security Best Practices:**
   - Use strong, unique passwords (minimum 12 characters)
   - Never commit `.env` or `src/config.h` to version control
   - Regularly update credentials
   - Use WPA2/WPA3 for WiFi encryption
   - Consider VPN for remote access

## 🏗️ **Infrastructure Overview**

```
ESP32 → WiFi → MQTT Broker → Database → Web Interface
                    ↓
              Real-time Logging
```

### Components
- **Mosquitto MQTT Broker**: Message routing and authentication
- **MariaDB Database**: Session and event data storage with proper timestamps
- **Enhanced Analytics**: Real-time movement tracking, performance monitoring, and clinical data collection
- **Web Dashboard**: Real-time data visualization and monitoring with WebSocket updates
- **Grafana Integration**: Professional analytics dashboards for clinical data visualization
- **phpMyAdmin**: Database management interface
- **MQTT Bridge**: Real-time data bridge with WebSocket support for live updates
- **Docker Compose**: Container orchestration
- **Future InfluxDB**: High-frequency sensor data storage for advanced analytics

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

**⚠️ IMPORTANT**: Before creating the compose.yaml, ensure you have configured your `.env` file with actual credentials.

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
      MYSQL_ROOT_PASSWORD: ${DB_ROOT_PASSWORD}
      MYSQL_DATABASE: ${DB_NAME}
      MYSQL_USER: ${DB_USER}
      MYSQL_PASSWORD: ${DB_PASSWORD}
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
      PMA_USER: ${DB_USER}
      PMA_PASSWORD: ${DB_PASSWORD}
      MYSQL_ROOT_PASSWORD: ${DB_ROOT_PASSWORD}
    ports:
      - 8080:80
    depends_on:
      - mariadb
    networks:
      - mqtt_network

  webapp:
    build: ./webapp
    container_name: rehab_webapp
    ports:
      - 3000:3000
    depends_on:
      - mariadb
    networks:
      - mqtt_network
    environment:
      - DB_HOST=mariadb
      - DB_USER=${DB_USER}
      - DB_PASS=${DB_PASSWORD}
      - DB_NAME=${DB_NAME}
    restart: unless-stopped

  mqtt-bridge:
    build: ./mqtt-bridge
    container_name: mqtt_bridge
    depends_on:
      - mariadb
      - mosquitto
    networks:
      - mqtt_network
    environment:
      - DB_HOST=mariadb
      - DB_USER=${DB_USER}
      - DB_PASS=${DB_PASSWORD}
      - DB_NAME=${DB_NAME}
      - MQTT_HOST=mosquitto
      - MQTT_USER=${MQTT_USER}
      - MQTT_PASSWORD=${MQTT_PASSWORD}
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    container_name: grafana_analytics
    ports:
      - 3001:3000
    depends_on:
      - mariadb
    networks:
      - mqtt_network
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=${GRAFANA_ADMIN_PASSWORD:-admin}
      - GF_INSTALL_PLUGINS=grafana-clock-panel,grafana-simple-json-datasource
    volumes:
      - grafana_data:/var/lib/grafana
      - ./grafana/provisioning:/etc/grafana/provisioning
    restart: unless-stopped

networks:
  mqtt_network:
    driver: bridge

volumes:
  mariadb_data:
  mariadb_config:
  sql_init:
  grafana_data:
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
# Create password file (replace 'your_mqtt_username' with your actual username)
sudo docker run --rm -v $(pwd)/config:/mosquitto/config eclipse-mosquitto mosquitto_passwd -c /mosquitto/config/pwfile your_mqtt_username

# Enter your secure password when prompted
```

Set permissions:
```bash
sudo chmod 0700 config/pwfile
```

### Step 4: Create Web Application Files

Copy the webapp directory from the project repository to your server:

**Option A: Direct SCP (if you have the project locally)**
```bash
# From your local machine, copy webapp directory
scp -r webapp/ user@your-server-ip:~/

# Then SSH into server and move to correct location
ssh user@your-server-ip
sudo mv ~/webapp /opt/stacks/mqtt/
sudo chown -R user:user /opt/stacks/mqtt/webapp
```

**Option B: Clone Repository on Server**
```bash
# SSH into your server
ssh user@your-server-ip

# Clone the repository
git clone https://github.com/your-username/ESP_Rehab_Hand_Exoskeleton.git
sudo cp -r ESP_Rehab_Hand_Exoskeleton/webapp /opt/stacks/mqtt/
sudo chown -R user:user /opt/stacks/mqtt/webapp
```

**Option C: Manual File Creation (if copying files individually)**
```bash
# Create webapp directory structure
sudo mkdir -p /opt/stacks/mqtt/webapp/public
sudo chown -R user:user /opt/stacks/mqtt/webapp

# Create the required files (copy content from repository):
# - webapp/Dockerfile
# - webapp/package.json
# - webapp/server.js
# - webapp/public/index.html
# - webapp/public/style.css
# - webapp/public/app.js
```

**Verify webapp structure:**
```bash
ls -la /opt/stacks/mqtt/webapp/
# Should show: Dockerfile, package.json, server.js, public/

ls -la /opt/stacks/mqtt/webapp/public/
# Should show: app.js, index.html, style.css
```

### Step 5: Create Database Schema

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
    session_type ENUM('unknown', 'sequential', 'simultaneous', 'mixed', 'test') DEFAULT 'unknown',
    session_status ENUM('active', 'completed', 'interrupted') DEFAULT 'active',
    total_movements INT DEFAULT 0,
    successful_movements INT DEFAULT 0,
    total_cycles INT DEFAULT 0,
    end_reason VARCHAR(50) DEFAULT 'user_requested',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_session_id (session_id),
    INDEX idx_device_id (device_id),
    INDEX idx_start_time (start_time),
    INDEX idx_session_status (session_status)
);

-- Events table for detailed logging (Enhanced with analytics support)
CREATE TABLE IF NOT EXISTS events (
    id INT AUTO_INCREMENT PRIMARY KEY,
    session_id VARCHAR(50),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    event_type ENUM('session_start', 'session_end', 'session_interrupted', 'movement_command',
                   'movement_complete', 'ble_connect', 'ble_disconnect', 'system_error',
                   'movement_individual', 'movement_quality', 'performance_timing',
                   'performance_memory', 'clinical_progress', 'clinical_quality') NOT NULL,
    command VARCHAR(20),
    response_time_ms INT,
    servo_data JSON,
    error_message TEXT,
    cycles_completed INT DEFAULT 0,
    movement_successful BOOLEAN DEFAULT TRUE,
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

### Enhanced Analytics Tables (Future Expansion)

When integrating advanced sensors, these additional tables will be created:

```sql
-- Future sensor data table (for InfluxDB integration)
CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    session_id VARCHAR(50),
    device_id VARCHAR(50),
    sensor_type ENUM('heart_rate', 'motion', 'pressure', 'temperature'),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    raw_value FLOAT,
    processed_value FLOAT,
    unit VARCHAR(10),
    quality_score FLOAT,
    metadata JSON,
    INDEX idx_session_id (session_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_sensor_type (sensor_type)
);

-- Movement analytics table
CREATE TABLE IF NOT EXISTS movement_analytics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    session_id VARCHAR(50),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    servo_index INT,
    movement_type VARCHAR(50),
    start_angle INT,
    target_angle INT,
    actual_angle INT,
    duration_ms INT,
    successful BOOLEAN,
    smoothness_score FLOAT,
    range_of_motion FLOAT,
    force_applied FLOAT,
    quality_metrics JSON,
    INDEX idx_session_id (session_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_servo_index (servo_index)
);
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

# Test publish (replace with your actual MQTT credentials)
mosquitto_pub -h localhost -p 1883 -u your_mqtt_username -P your_mqtt_password -t "test/topic" -m "Hello MQTT"

# Test subscribe (in another terminal)
mosquitto_sub -h localhost -p 1883 -u your_mqtt_username -P your_mqtt_password -t "test/topic"
```

### Test Database Connection
```bash
# Connect to MariaDB (replace with your actual credentials)
sudo docker exec -it mariadb_mqtt mariadb -u your_db_username -pyour_db_password rehab_exoskeleton

# Verify tables (if init script didn't run, create manually)
SHOW TABLES;
```

### Manual Table Creation (if needed)
If tables don't exist, run the SQL from `sql_init/init_rehab_db.sql` manually in the MariaDB prompt.

### Test Web Dashboard
```bash
# Check if webapp is running
curl http://localhost:3000

# Check API endpoints
curl http://localhost:3000/api/status
curl http://localhost:3000/api/sessions
```

Open your browser and navigate to `http://your-server-ip:3000` to access the dashboard.

### Test Grafana Analytics
```bash
# Check if Grafana is running
curl http://localhost:3001

# Check Grafana health
curl http://localhost:3001/api/health
```

Open your browser and navigate to `http://your-server-ip:3001` to access Grafana:
1. **Login**: admin / your_grafana_password (from .env file)
2. **Add Data Source**:
   - Type: MySQL
   - Host: mariadb:3306
   - Database: rehab_exoskeleton
   - User: your_db_username
   - Password: your_db_password
3. **Import Dashboards**: Use queries from [DATABASE_QUERIES.md](DATABASE_QUERIES.md)

**Expected Dashboard Features:**
- 📊 **System Overview**: Total sessions, today's activity, total duration, total cycles
- 🔌 **Device Status**: ESP32 online/offline status and health metrics
- 📋 **Recent Sessions**: List of therapy sessions with accurate duration and cycle tracking
- ⚡ **Recent Events**: Real-time activity log including session start/end events
- 📡 **Live Monitor**: Real-time updates when ESP32 is active
- 🎯 **Session Management**: Proper session tracking with manual termination support
- 📈 **Cycle Tracking**: Accurate movement cycle counting and progress monitoring

**Initial State**: The dashboard will show empty data until ESP32 starts sending MQTT data.

**Recent Improvements:**
- ✅ **Fixed Duration Calculation**: Sessions now show accurate durations (e.g., "5m 30s" instead of "3117h")
- ✅ **Cycle Tracking**: Total cycles are now properly tracked and displayed
- ✅ **Manual Session End**: "End Session" button in mobile app properly terminates sessions
- ✅ **Real-time Statistics**: Dashboard updates immediately when sessions end

---

## 🌐 **Access Points**

| Service | URL/Address | Credentials |
|---------|-------------|-------------|
| **MQTT Broker** | `localhost:1883` | your_mqtt_username / your_mqtt_password |
| **MQTT WebSocket** | `localhost:9001` | your_mqtt_username / your_mqtt_password |
| **MariaDB** | `localhost:3306` | your_db_username / your_db_password |
| **Web Dashboard** | `http://localhost:3000` | None (public) |
| **Grafana Analytics** | `http://localhost:3001` | admin / your_grafana_password |
| **phpMyAdmin** | `http://localhost:8080` | your_db_username / your_db_password |

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
# Connect to database (replace with your actual credentials)
sudo docker exec -it mariadb_mqtt mariadb -u your_db_username -pyour_db_password rehab_exoskeleton

# Backup database
sudo docker exec mariadb_mqtt mysqldump -u your_db_username -pyour_db_password rehab_exoskeleton > backup.sql

# Restore database
sudo docker exec -i mariadb_mqtt mysql -u your_db_username -pyour_db_password rehab_exoskeleton < backup.sql

# Clear all data (for fresh start)
sudo docker exec -it mariadb_mqtt mariadb -u your_db_username -pyour_db_password rehab_exoskeleton -e "
DELETE FROM sessions;
DELETE FROM events;
DELETE FROM system_status;
ALTER TABLE sessions AUTO_INCREMENT = 1;
ALTER TABLE events AUTO_INCREMENT = 1;
ALTER TABLE system_status AUTO_INCREMENT = 1;
"
```

---

## 🛡️ **Security Considerations**

### Current Setup (Development)
- **MQTT Authentication**: Username/password (configured in .env)
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

# Verify database exists (replace with your actual root password)
sudo docker exec -it mariadb_mqtt mariadb -u root -pyour_root_password -e "SHOW DATABASES;"
```

**Web app build fails with npm ci error:**
```bash
# Error: npm ci requires package-lock.json
# Solution: Update Dockerfile to use npm install instead

# Edit the Dockerfile
sudo nano webapp/Dockerfile

# Change this line:
# RUN npm ci --only=production
# To this:
# RUN npm install --only=production

# Then rebuild
sudo docker-compose down
sudo docker-compose up -d
```

**Web app container won't start:**
```bash
# Check webapp logs
sudo docker-compose logs webapp

# Common issues:
# 1. Database connection - ensure MariaDB is running first
# 2. Port conflict - ensure port 3000 is available
# 3. File permissions - ensure webapp files are readable

# Check if port 3000 is in use
sudo netstat -tlnp | grep 3000
```

### Log Locations
- **Mosquitto logs**: `./log/mosquitto.log`
- **MariaDB logs**: `sudo docker-compose logs mariadb`
- **Container logs**: `sudo docker-compose logs [service_name]`

---

## 📊 **Next Steps**

After successful setup:
1. ✅ **Infrastructure Complete**: MQTT broker, database, web dashboard, and Grafana analytics running
2. ✅ **Enhanced Analytics**: ESP32 firmware now includes detailed movement tracking and performance monitoring
3. 🔄 **Grafana Dashboard Setup**: Configure dashboards using queries from [DATABASE_QUERIES.md](DATABASE_QUERIES.md)
4. 🔄 **Mobile App Integration**: Add data logging to React Native app
5. 🔄 **Advanced Sensor Integration**: Add pressure sensors, MPU-6050, and MAX30102 for comprehensive analytics
6. 🔄 **InfluxDB Integration**: High-frequency sensor data storage for real-time analytics
7. 🔄 **GraphDB Integration**: Advanced visual data interpretation (future enhancement)

**Current Status**: Backend infrastructure with enhanced analytics ready to receive detailed ESP32 data.
**Next Priority**: Set up Grafana dashboards and test enhanced movement analytics.

---

## 📝 **Configuration Summary**

**MQTT Topics Structure:**
```
rehab_exo/ESP32_001/
├── session/
│   ├── start                    # Session start events
│   ├── end                      # Session end events
│   └── progress                 # Session progress updates
├── movement/
│   ├── command                  # Movement commands and responses
│   ├── individual               # Individual servo movement data (NEW)
│   └── quality                  # Movement quality metrics (NEW)
├── performance/
│   ├── timing                   # System performance timing (NEW)
│   └── memory                   # Memory usage analytics (NEW)
├── clinical/
│   ├── progress                 # Clinical progress tracking (NEW)
│   └── quality                  # Clinical session quality (NEW)
├── system/
│   └── status                   # System health and performance
└── connection/
    ├── wifi                     # WiFi connection events
    └── ble                      # BLE connection events
```

**Future Sensor Topics (InfluxDB Integration):**
```
rehab_exo/ESP32_001/sensors/
├── heart_rate                   # GY-MAX30102 Heart Rate & SpO2
├── motion                       # GY-521 MPU-6050 Motion Data
├── pressure                     # Force Sensor Data
└── analytics/
    └── movement                 # Real-time movement analysis
```

**Database Schema:**
- **sessions**: Therapy session tracking
- **events**: Detailed command/response logging (enhanced with analytics events)
- **system_status**: Device health monitoring
- **sensor_data**: High-frequency sensor data (future InfluxDB integration)
- **movement_analytics**: Detailed movement analysis (future expansion)

**Network Configuration:**
- **Internal Network**: `mqtt_network` (Docker bridge)
- **External Ports**: 1883 (MQTT), 9001 (WebSocket), 3306 (MariaDB), 3000 (Web Dashboard), 3001 (Grafana), 8080 (phpMyAdmin)
