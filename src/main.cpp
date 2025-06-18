#include <Arduino.h>
#include "app/DeviceManager.h"
#include "utils/Logger.h"
#include "config/Config.h"

// Global device manager instance
DeviceManager deviceManager;

// Performance tracking
unsigned long lastLoopTime = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3000) {
        vTaskDelay(pdMS_TO_TICKS(10));  // FreeRTOS delay
    }

    // Initialize logger first
    Logger::initialize(LogLevel::INFO);

    // Print startup banner
    Logger::info("=== ESP32 Rehabilitation Hand Exoskeleton ===");
    Logger::infof("Firmware Version: %s", FIRMWARE_VERSION);
    Logger::infof("Device ID: %s", DEVICE_ID);
    Logger::info("Starting system initialization...");

    // Initialize the device manager (handles all subsystems)
    deviceManager.initialize();

    if (deviceManager.isReady()) {
        Logger::info("=== System Ready ===");
        Logger::info("Device is ready to accept commands");
        Logger::info("Available interfaces: BLE, WiFi/MQTT");
        Logger::info("Send commands: 0 (home), 1 (sequential), 2 (simultaneous)");
    } else {
        Logger::error("=== System Initialization Failed ===");
        Logger::error("Check configuration and hardware connections");
    }

    lastLoopTime = millis();
}

void loop() {
    // ✅ PURE FREERTOS ARCHITECTURE
    // Main loop is now minimal - all work done by FreeRTOS tasks:
    // - DeviceManager Task (Core 1, Priority 3) - System coordination
    // - WiFiManager Task (Core 0, Priority 5) - Network connectivity
    // - MQTTManager Tasks (Core 0, Priority 4) - Data publishing
    // - BLEManager Task (Core 0, Priority 3) - Mobile connectivity
    // - ServoController Task (Core 1, Priority 6) - Movement control
    // - I2CManager Task (Core 1, Priority 5) - Sensor communication
    // - PulseMonitor Task (Core 1, Priority 4) - Health monitoring
    // - SessionAnalytics Task (Core 1, Priority 2) - Data processing

    // Handle serial debugging only (minimal for development)
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.length() > 0) {
            Logger::infof("Serial debug: %s", command.c_str());

            // Debug commands - real work done by tasks
            if (command == "status") {
                deviceManager.logComponentStatus();
            } else if (command == "freertos") {
                deviceManager.logFreeRTOSStatus();
            } else if (command == "tasks") {
                Logger::infof("FreeRTOS Tasks: %u", uxTaskGetNumberOfTasks());
                Logger::infof("Free Heap: %u bytes", ESP.getFreeHeap());
                Logger::infof("Min Free Heap: %u bytes", ESP.getMinFreeHeap());
            } else {
                // Pass to DeviceManager task for processing
                deviceManager.handleCommand(command, CommandSource::SERIAL_PORT);
            }
        }
    }

    // FreeRTOS scheduler handles everything - main loop just sleeps
    vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second - FreeRTOS delay
}
