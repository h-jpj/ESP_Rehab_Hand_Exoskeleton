# Database Queries & Grafana Configuration

This file contains all the database schemas, queries, and Grafana configurations for the ESP32 Rehabilitation Hand Exoskeleton project. Use this as a reference to quickly set up your analytics without having to write queries from scratch.

## 📋 **Table of Contents**
- [MariaDB Database Schema](#mariadb-database-schema)
- [Grafana Queries](#grafana-queries)
- [Web Application Queries](#web-application-queries)
- [MQTT Topics Reference](#mqtt-topics-reference)
- [Future InfluxDB Schema](#future-influxdb-schema)

---

## 🗄️ **MariaDB Database Schema**

### **Core Tables**

#### **sessions table**
```sql
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
```

#### **events table**
```sql
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
```

#### **system_status table**
```sql
CREATE TABLE IF NOT EXISTS system_status (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_id VARCHAR(50) NOT NULL,
    status ENUM('online', 'offline', 'error') NOT NULL,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(45),
    firmware_version VARCHAR(20),
    uptime_seconds INT,
    free_heap INT,
    wifi_rssi INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_device_id (device_id),
    INDEX idx_last_seen (last_seen)
);
```

### **Enhanced Analytics Tables (Future)**

#### **sensor_data table** (For InfluxDB integration)
```sql
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
```

#### **movement_analytics table**
```sql
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

## 📊 **Grafana Queries**

### **Row 1: Overview Stats**

#### **Active Sessions (Stat Panel)**
```sql
SELECT COUNT(*) as value
FROM sessions 
WHERE session_status = 'active'
```

#### **Today's Sessions (Stat Panel)**
```sql
SELECT COUNT(*) as value
FROM sessions 
WHERE DATE(start_time) = CURDATE()
```

#### **Success Rate (Stat Panel)**
```sql
SELECT 
    ROUND(
        (SUM(successful_movements) / NULLIF(SUM(total_movements), 0)) * 100, 1
    ) as value
FROM sessions 
WHERE session_status IN ('completed', 'interrupted')
    AND start_time >= DATE_SUB(NOW(), INTERVAL 7 DAY)
```

#### **System Status (Stat Panel)**
```sql
SELECT 
    CASE 
        WHEN status = 'online' THEN 1
        WHEN status = 'offline' THEN 0
        ELSE -1
    END as value,
    device_id as metric
FROM system_status 
ORDER BY last_seen DESC 
LIMIT 1
```

### **Row 2: Session Analytics**

#### **Session Activity Over Time (Time Series)**
```sql
SELECT 
    start_time as time,
    COUNT(*) as sessions_count
FROM sessions 
WHERE start_time >= $__timeFrom() AND start_time <= $__timeTo()
GROUP BY DATE_FORMAT(start_time, '%Y-%m-%d %H:00:00')
ORDER BY time
```

#### **Session Types Distribution (Pie Chart)**
```sql
SELECT 
    session_type as metric,
    COUNT(*) as value
FROM sessions 
WHERE start_time >= $__timeFrom() AND start_time <= $__timeTo()
GROUP BY session_type
```

### **Row 3: Performance Metrics**

#### **Success Rate Trends (Time Series)**
```sql
SELECT 
    start_time as time,
    ROUND(
        (SUM(successful_movements) / NULLIF(SUM(total_movements), 0)) * 100, 1
    ) as success_rate
FROM sessions 
WHERE start_time >= $__timeFrom() AND start_time <= $__timeTo()
    AND session_status IN ('completed', 'interrupted')
GROUP BY DATE_FORMAT(start_time, '%Y-%m-%d %H:00:00')
ORDER BY time
```

#### **Response Times (Time Series)**
```sql
SELECT 
    timestamp as time,
    response_time_ms as value
FROM events 
WHERE timestamp >= $__timeFrom() AND timestamp <= $__timeTo()
    AND event_type = 'movement_command'
    AND response_time_ms IS NOT NULL
ORDER BY time
```

### **Row 4: System Health**

#### **Memory Usage (Time Series)**
```sql
SELECT 
    last_seen as time,
    free_heap as free_memory,
    (320000 - free_heap) as used_memory
FROM system_status 
WHERE last_seen >= $__timeFrom() AND last_seen <= $__timeTo()
ORDER BY time
```

#### **Connection Status (State Timeline)**
```sql
SELECT 
    last_seen as time,
    CASE 
        WHEN status = 'online' THEN 1
        WHEN status = 'offline' THEN 0
        ELSE -1
    END as value,
    device_id as metric
FROM system_status 
WHERE last_seen >= $__timeFrom() AND last_seen <= $__timeTo()
ORDER BY time
```

### **Row 5: Clinical Data**

#### **Therapy Progress (Multi-line Time Series)**
```sql
SELECT 
    start_time as time,
    AVG(duration_seconds) as avg_duration,
    AVG(total_movements) as avg_movements,
    AVG(total_cycles) as avg_cycles
FROM sessions 
WHERE start_time >= $__timeFrom() AND start_time <= $__timeTo()
    AND session_status IN ('completed', 'interrupted')
GROUP BY DATE_FORMAT(start_time, '%Y-%m-%d')
ORDER BY time
```

#### **Movement Commands (Stacked Bars)**
```sql
SELECT 
    DATE_FORMAT(timestamp, '%Y-%m-%d') as time,
    command as metric,
    COUNT(*) as value
FROM events 
WHERE timestamp >= $__timeFrom() AND timestamp <= $__timeTo()
    AND event_type = 'movement_command'
    AND command IN ('1', '2', 'TEST')
GROUP BY DATE_FORMAT(timestamp, '%Y-%m-%d'), command
ORDER BY time
```

---

## 🌐 **Web Application Queries**

### **Dashboard Statistics**
```sql
-- Overall Statistics
SELECT
    COUNT(*) as total_sessions,
    SUM(COALESCE(duration_seconds, 0)) as total_duration,
    AVG(COALESCE(duration_seconds, 0)) as avg_duration,
    SUM(COALESCE(total_cycles, 0)) as total_cycles,
    SUM(COALESCE(total_movements, 0)) as total_movements,
    SUM(COALESCE(successful_movements, 0)) as successful_movements
FROM sessions
WHERE session_status IN ('completed', 'interrupted');

-- Today's Statistics
SELECT
    COUNT(*) as today_sessions,
    SUM(COALESCE(duration_seconds, 0)) as today_duration,
    SUM(COALESCE(total_cycles, 0)) as today_cycles
FROM sessions
WHERE DATE(start_time) = CURDATE()
    AND session_status IN ('completed', 'interrupted');
```

### **Recent Sessions**
```sql
SELECT
    id, session_id, device_id, user_id,
    start_time, end_time, duration_seconds,
    session_type, session_status,
    total_movements, successful_movements, total_cycles,
    end_reason, created_at
FROM sessions
ORDER BY start_time DESC
LIMIT 50;
```

### **Session Details**
```sql
-- Session Information
SELECT * FROM sessions WHERE session_id = ?;

-- Session Events
SELECT
    id, timestamp, event_type, command,
    response_time_ms, servo_data, error_message
FROM events
WHERE session_id = ?
ORDER BY timestamp ASC;
```

### **Recent Events**
```sql
SELECT
    e.id, e.session_id, e.timestamp, e.event_type,
    e.command, e.response_time_ms,
    s.session_type, s.device_id
FROM events e
LEFT JOIN sessions s ON e.session_id = s.session_id
ORDER BY e.timestamp DESC
LIMIT 20;
```

---

## 📡 **MQTT Topics Reference**

### **Current Topics**
```
rehab_exo/ESP32_001/movement/command      # Movement commands and responses
rehab_exo/ESP32_001/system/status        # System health and performance
rehab_exo/ESP32_001/connection/wifi      # WiFi connection events
rehab_exo/ESP32_001/connection/ble       # BLE connection events
rehab_exo/ESP32_001/session/start        # Session start events
rehab_exo/ESP32_001/session/end          # Session end events
rehab_exo/ESP32_001/session/progress     # Session progress updates
```

### **Enhanced Analytics Topics**
```
rehab_exo/ESP32_001/movement/individual  # Individual servo movement data
rehab_exo/ESP32_001/movement/quality     # Movement quality metrics
rehab_exo/ESP32_001/performance/timing   # System performance timing
rehab_exo/ESP32_001/performance/memory   # Memory usage analytics
rehab_exo/ESP32_001/clinical/progress    # Clinical progress tracking
rehab_exo/ESP32_001/clinical/quality     # Clinical session quality
```

### **Future Sensor Topics (InfluxDB)**
```
rehab_exo/ESP32_001/sensors/heart_rate   # Heart rate and SpO2 data
rehab_exo/ESP32_001/sensors/motion       # MPU-6050 motion data
rehab_exo/ESP32_001/sensors/pressure     # Force sensor data
rehab_exo/ESP32_001/analytics/movement   # Real-time movement analysis
```

---

## ⚡ **Future InfluxDB Schema**

### **Measurement: sensor_readings**
```
sensor_readings,device=ESP32_001,sensor=heart_rate value=72,quality=0.95 1640995200000000000
sensor_readings,device=ESP32_001,sensor=motion_x value=0.15,quality=1.0 1640995200000000000
sensor_readings,device=ESP32_001,sensor=pressure value=150.5,quality=0.88 1640995200000000000
```

### **Measurement: movement_metrics**
```
movement_metrics,device=ESP32_001,servo=0 angle=45,smoothness=0.92,duration=1050 1640995200000000000
movement_metrics,device=ESP32_001,servo=1 angle=60,smoothness=0.87,duration=1120 1640995200000000000
```

### **Measurement: system_performance**
```
system_performance,device=ESP32_001 cpu_usage=15.2,memory_free=45000,loop_time=25 1640995200000000000
```

---

## 🔧 **Quick Setup Commands**

### **Create Database and Tables**
```bash
# Connect to MariaDB
sudo docker exec -it mariadb_mqtt mariadb -u jay -paes_root_2024 rehab_exoskeleton

# Run the schema creation queries above
```

### **Test Queries**
```bash
# Test basic functionality
SELECT COUNT(*) FROM sessions;
SELECT COUNT(*) FROM events;
SELECT COUNT(*) FROM system_status;
```

### **Grafana Data Source Configuration**
```
Host: mariadb:3306
Database: rehab_exoskeleton
User: your_username
Password: your_password
```

This reference file eliminates the need to write queries from scratch and provides a complete foundation for setting up the rehabilitation analytics system.
