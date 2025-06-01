#ifndef CONFIG_H
#define CONFIG_H

#include <cstddef>  // For size_t

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
const int MQTT_BUFFER_SIZE = 1024;
const size_t MIN_FREE_HEAP = 10000;  // Minimum free heap in bytes

// Error Handling
const int MAX_WIFI_RETRIES = 5;
const int MAX_MQTT_RETRIES = 3;
const unsigned long WATCHDOG_TIMEOUT = 30000;  // 30 seconds

// Power Management
const int DEEP_SLEEP_DURATION = 60;  // seconds (for future use)

#endif
