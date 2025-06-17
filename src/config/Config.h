#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>  // For size_t
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

// ⚠️  SECURITY NOTICE ⚠️
// This file contains your actual WiFi and server credentials
// Make sure this file is in .gitignore and never committed to version control
// See config.h.example for template

// WiFi Configuration
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// MQTT Configuration
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASSWORD;

// Device Configuration
extern const char* DEVICE_ID;
extern const char* FIRMWARE_VERSION;

// MQTT Topics
extern const char* TOPIC_BASE;
extern const char* TOPIC_MOVEMENT_COMMAND;
extern const char* TOPIC_SYSTEM_STATUS;
extern const char* TOPIC_CONNECTION_WIFI;
extern const char* TOPIC_CONNECTION_BLE;
extern const char* TOPIC_SESSION_START;
extern const char* TOPIC_SESSION_END;
extern const char* TOPIC_SESSION_PROGRESS;

// Enhanced Analytics Topics
extern const char* TOPIC_MOVEMENT_INDIVIDUAL;
extern const char* TOPIC_MOVEMENT_QUALITY;
extern const char* TOPIC_PERFORMANCE_TIMING;
extern const char* TOPIC_PERFORMANCE_MEMORY;
extern const char* TOPIC_CLINICAL_PROGRESS;
extern const char* TOPIC_CLINICAL_QUALITY;

// Future Sensor Topics (InfluxDB Ready)
extern const char* TOPIC_SENSOR_HEART_RATE;
extern const char* TOPIC_SENSOR_MOTION;
extern const char* TOPIC_SENSOR_PRESSURE;
extern const char* TOPIC_SENSOR_ANALYTICS;

// Timing Configuration
const unsigned long WIFI_RECONNECT_INTERVAL = 30000;  // 30 seconds
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;   // 5 seconds
const unsigned long STATUS_REPORT_INTERVAL = 2000;    // 2 seconds

// Servo Configuration
const int SERVO_PINS[3] = {19, 22, 23};
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 90;
const int SERVO_MOVEMENT_DELAY = 1000;  // milliseconds
const int SERVO_CYCLES = 3;

// BLE Configuration
extern const char* BLE_SERVICE_UUID;
extern const char* BLE_CHARACTERISTIC_UUID;
extern const char* BLE_DEVICE_NAME;

// System Configuration
const int SERIAL_BAUD_RATE = 115200;
const int MQTT_BUFFER_SIZE = 8192;  // Increased to 8KB to fix persistent corruption
const size_t MIN_FREE_HEAP = 10000;  // Minimum free heap in bytes

// Error Handling
const int MAX_WIFI_RETRIES = 5;
const int MAX_MQTT_RETRIES = 3;
const unsigned long WATCHDOG_TIMEOUT = 30000;  // 30 seconds

// Power Management
const int DEEP_SLEEP_DURATION = 60;  // seconds (for future use)

// =============================================================================
// FREERTOS TASK CONFIGURATION
// =============================================================================

// Task Stack Sizes (in words, not bytes)
const uint32_t TASK_STACK_WIFI_MANAGER = 4096;
const uint32_t TASK_STACK_MQTT_PUBLISHER = 6144;
const uint32_t TASK_STACK_MQTT_SUBSCRIBER = 4096;
const uint32_t TASK_STACK_BLE_SERVER = 8192;
const uint32_t TASK_STACK_NETWORK_WATCHDOG = 4096;
const uint32_t TASK_STACK_SERVO_CONTROL = 4096;
const uint32_t TASK_STACK_I2C_MANAGER = 4096;
const uint32_t TASK_STACK_PULSE_MONITOR = 4096;
const uint32_t TASK_STACK_MOTION_SENSOR = 4096;
const uint32_t TASK_STACK_PRESSURE_SENSOR = 4096;
const uint32_t TASK_STACK_DATA_FUSION = 5120;
const uint32_t TASK_STACK_SESSION_ANALYTICS = 4096;
const uint32_t TASK_STACK_SYSTEM_HEALTH = 4096;

// Task Priorities (0 = lowest, configMAX_PRIORITIES-1 = highest)
// Core 0 (Protocol CPU) - Communication Tasks
const UBaseType_t PRIORITY_WIFI_MANAGER = 5;
const UBaseType_t PRIORITY_MQTT_PUBLISHER = 4;
const UBaseType_t PRIORITY_MQTT_SUBSCRIBER = 4;
const UBaseType_t PRIORITY_BLE_SERVER = 3;
const UBaseType_t PRIORITY_NETWORK_WATCHDOG = 2;

// Core 1 (Application CPU) - Control & Sensing Tasks
const UBaseType_t PRIORITY_SERVO_CONTROL = 6;      // Highest - safety critical
const UBaseType_t PRIORITY_I2C_MANAGER = 5;        // Hardware coordination
const UBaseType_t PRIORITY_PULSE_MONITOR = 4;      // Medical priority
const UBaseType_t PRIORITY_MOTION_SENSOR = 4;      // Movement analysis
const UBaseType_t PRIORITY_PRESSURE_SENSOR = 3;    // Force feedback
const UBaseType_t PRIORITY_DATA_FUSION = 3;        // Sensor correlation
const UBaseType_t PRIORITY_SESSION_ANALYTICS = 2;  // Clinical analysis
const UBaseType_t PRIORITY_SYSTEM_HEALTH = 1;      // Background monitoring

// Core Assignment
const BaseType_t CORE_PROTOCOL = 0;     // WiFi, MQTT, BLE
const BaseType_t CORE_APPLICATION = 1;  // Sensors, Control, Analytics

// Queue Sizes
const UBaseType_t QUEUE_SIZE_PULSE_RAW = 100;        // 25Hz * 4 seconds
const UBaseType_t QUEUE_SIZE_MOTION_RAW = 200;       // 100Hz * 2 seconds
const UBaseType_t QUEUE_SIZE_PRESSURE_RAW = 50;      // 10Hz * 5 seconds
const UBaseType_t QUEUE_SIZE_PULSE_PROCESSED = 20;   // 1Hz * 20 seconds
const UBaseType_t QUEUE_SIZE_MOTION_PROCESSED = 50;  // 10Hz * 5 seconds
const UBaseType_t QUEUE_SIZE_PRESSURE_PROCESSED = 10; // 1Hz * 10 seconds
const UBaseType_t QUEUE_SIZE_SERVO_COMMANDS = 10;    // Movement commands
const UBaseType_t QUEUE_SIZE_I2C_REQUESTS = 20;      // I2C bus requests
const UBaseType_t QUEUE_SIZE_MQTT_PUBLISH = 50;      // MQTT messages
const UBaseType_t QUEUE_SIZE_SESSION_EVENTS = 20;    // Session events
const UBaseType_t QUEUE_SIZE_SYSTEM_ALERTS = 15;     // System alerts
const UBaseType_t QUEUE_SIZE_FUSED_DATA = 30;        // Combined sensor data
const UBaseType_t QUEUE_SIZE_MOVEMENT_ANALYTICS = 25; // Movement analysis
const UBaseType_t QUEUE_SIZE_CLINICAL_DATA = 15;     // Clinical metrics
const UBaseType_t QUEUE_SIZE_PERFORMANCE_METRICS = 20; // Performance data

// Sensor Sampling Configuration
const uint32_t PULSE_SAMPLING_RATE_HZ = 25;          // 25Hz for heart rate
const uint32_t MOTION_SAMPLING_RATE_HZ = 100;        // 100Hz for motion
const uint32_t PRESSURE_SAMPLING_RATE_HZ = 10;       // 10Hz for pressure
const uint32_t PULSE_REPORTING_RATE_HZ = 1;          // 1Hz reporting
const uint32_t MOTION_REPORTING_RATE_HZ = 10;        // 10Hz reporting
const uint32_t PRESSURE_REPORTING_RATE_HZ = 1;       // 1Hz reporting

// I2C Configuration (Updated for servo compatibility)
const uint8_t I2C_SDA_PIN = 18;  // SDA pin for heart rate sensor
const uint8_t I2C_SCL_PIN = 21;  // SCL pin for heart rate sensor
const uint8_t I2C_INT_PIN = 4;   // Interrupt pin for heart rate sensor
const uint32_t I2C_CLOCK_SPEED = 400000;  // 400kHz fast mode
const uint32_t I2C_TIMEOUT_MS = 1000;

// Sensor I2C Addresses
const uint8_t I2C_ADDR_MAX30102 = 0x57;   // Pulse oximeter
const uint8_t I2C_ADDR_MPU6050 = 0x68;    // Motion sensor

// Event Group Bit Definitions
// Sensor Status Events
const EventBits_t EVENT_PULSE_SENSOR_READY = (1 << 0);
const EventBits_t EVENT_MOTION_SENSOR_READY = (1 << 1);
const EventBits_t EVENT_PRESSURE_SENSOR_READY = (1 << 2);
const EventBits_t EVENT_I2C_BUS_HEALTHY = (1 << 3);
const EventBits_t EVENT_CALIBRATION_COMPLETE = (1 << 4);
const EventBits_t EVENT_SESSION_ACTIVE = (1 << 5);
const EventBits_t EVENT_EMERGENCY_MODE = (1 << 6);
const EventBits_t EVENT_MQTT_CONNECTED = (1 << 7);

// System State Events
const EventBits_t EVENT_WIFI_CONNECTED = (1 << 0);
const EventBits_t EVENT_BLE_CLIENT_CONNECTED = (1 << 1);
const EventBits_t EVENT_SERVO_INITIALIZED = (1 << 2);
const EventBits_t EVENT_SENSORS_CALIBRATED = (1 << 3);
const EventBits_t EVENT_SYSTEM_READY = (1 << 4);

// Timing Constants
const TickType_t TASK_DELAY_PULSE_SAMPLING = pdMS_TO_TICKS(1000 / PULSE_SAMPLING_RATE_HZ);
const TickType_t TASK_DELAY_MOTION_SAMPLING = pdMS_TO_TICKS(1000 / MOTION_SAMPLING_RATE_HZ);
const TickType_t TASK_DELAY_PRESSURE_SAMPLING = pdMS_TO_TICKS(1000 / PRESSURE_SAMPLING_RATE_HZ);
const TickType_t TASK_DELAY_DATA_FUSION = pdMS_TO_TICKS(100);  // 10Hz fusion rate
const TickType_t TASK_DELAY_SYSTEM_HEALTH = pdMS_TO_TICKS(5000); // 5 second health checks

// Memory Management
const size_t SENSOR_DATA_POOL_SIZE = 8192;   // 8KB pool for sensor data
const size_t SENSOR_DATA_POOL_ITEMS = 64;    // Number of allocatable items
const size_t MAX_MQTT_MESSAGE_SIZE = 1024;   // Maximum MQTT message size

// Performance Monitoring
const uint32_t MAX_TASK_EXECUTION_TIME_MS = 100;  // Maximum task execution time
const uint32_t WATCHDOG_FEED_INTERVAL_MS = 1000;  // Watchdog feed interval

#endif
