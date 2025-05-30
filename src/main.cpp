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
        delay(10);  // Wait up to 3 seconds for serial
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
    unsigned long loopStart = millis();

    // Update the device manager (handles all subsystem updates)
    deviceManager.update();

    // Handle any serial commands for debugging
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.length() > 0) {
            Logger::infof("Serial command received: %s", command.c_str());

            // Handle session testing commands
            if (command == "session_info") {
                SessionManager& sm = deviceManager.getSessionManager();
                Logger::infof("=== SESSION INFO ===");
                Logger::infof("Active: %s", sm.isSessionActive() ? "Yes" : "No");
                if (sm.isSessionActive()) {
                    Logger::infof("ID: %s", sm.getCurrentSessionId().c_str());
                    Logger::infof("Type: %d", (int)sm.getCurrentType());
                    SessionStats stats = sm.getSessionStats();
                    Logger::infof("Duration: %lu ms", stats.duration);
                    Logger::infof("Movements: %d/%d", stats.successfulMovements, stats.totalMovements);
                }
                Logger::infof("==================");
            } else if (command == "start_session") {
                deviceManager.getSessionManager().startSession(false);
            } else if (command == "end_session") {
                deviceManager.getSessionManager().endSession("manual_test");
            } else {
                // Pass other commands to device manager
                deviceManager.handleCommand(command, CommandSource::SERIAL_PORT);
            }
        }
    }

    // Small delay to prevent overwhelming the system
    delay(10);

    // Track loop performance (optional - can be removed for production)
    unsigned long loopTime = millis() - loopStart;
    if (loopTime > 50) {  // Log if loop takes more than 50ms
        Logger::warningf("Long loop detected: %lu ms", loopTime);
    }
}
