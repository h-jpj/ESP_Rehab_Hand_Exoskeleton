#include "DeviceManager.h"
#include "../config/Config.h"
#include "CommandProcessor.h"

// Static instance for callbacks
DeviceManager* DeviceManager::instance = nullptr;

void DeviceManager::initialize() {
    if (initialized) return;

    // Set static instance for callbacks
    instance = this;

    Logger::info("=== ESP32 Rehabilitation Hand Exoskeleton ===");
    Logger::info("Initializing Device Manager...");

    currentState = DeviceState::INITIALIZING;
    lastStatusReport = 0;
    statusReportInterval = DEFAULT_STATUS_INTERVAL;
    lastUpdate = 0;
    loopStartTime = 0;
    totalLoopTime = 0;
    loopCount = 0;
    stateChangeCallback = nullptr;
    taskHandle = nullptr;
    taskRunning = false;

    // Initialize in phases
    if (!initializeFoundation()) {
        setState(DeviceState::ERROR);
        return;
    }

    // Communication initialization (WiFi/MQTT) is optional for basic operation
    initializeCommunication();  // Don't fail if this doesn't work

    if (!initializeHardware()) {
        setState(DeviceState::ERROR);
        return;
    }

    if (!initializeApplication()) {
        setState(DeviceState::ERROR);
        return;
    }

    initialized = true;
    setState(DeviceState::READY);

    Logger::info("Device ready for BLE commands (WiFi/MQTT optional)");

    // Start the DeviceManager coordination task
    startTask();

    Logger::info("=== Device Manager Initialization Complete ===");
    logSystemSummary();
}

void DeviceManager::update() {
    if (!initialized) return;

    loopStartTime = millis();

    // Update all components
    updateCommunication();
    updateHardware();
    updateApplication();
    updateMonitoring();

    // Process any queued commands
    processQueuedCommands();

    // Periodic status reporting
    unsigned long now = millis();
    if (now - lastStatusReport >= statusReportInterval) {
        publishSystemStatus();
        lastStatusReport = now;
    }

    // Check system health
    checkSystemHealth();

    // Record performance metrics
    unsigned long loopTime = millis() - loopStartTime;
    totalLoopTime += loopTime;
    loopCount++;
    systemMonitor.recordLoopTime(loopTime);

    if (loopTime > MAX_LOOP_TIME) {
        Logger::warningf("Long loop time detected: %lu ms", loopTime);
    }

    lastUpdate = now;
}

void DeviceManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down Device Manager...");

    setState(DeviceState::MAINTENANCE);

    // Shutdown components in reverse order
    servoController.shutdown();
    bleManager.shutdown();
    mqttManager.disconnect();
    wifiManager.disconnect();

    // TODO: Re-enable when FreeRTOS Manager is active
    // Shutdown FreeRTOS infrastructure last
    // I2CManager::shutdown();
    // FreeRTOSManager::shutdown();

    initialized = false;
    instance = nullptr;

    Logger::info("Device Manager shutdown complete");
}

DeviceState DeviceManager::getState() {
    return currentState;
}

bool DeviceManager::isReady() {
    return currentState == DeviceState::READY || currentState == DeviceState::RUNNING;
}

bool DeviceManager::isHealthy() {
    return systemMonitor.isSystemHealthy() && !ErrorHandler::hasCriticalErrors();
}

WiFiManager& DeviceManager::getWiFiManager() {
    return wifiManager;
}

MQTTManager& DeviceManager::getMQTTManager() {
    return mqttManager;
}



BLEManager& DeviceManager::getBLEManager() {
    return bleManager;
}

ServoController& DeviceManager::getServoController() {
    return servoController;
}

SystemMonitor& DeviceManager::getSystemMonitor() {
    return systemMonitor;
}

// SystemHealthManager removed - using SystemMonitor instead

SessionAnalyticsManager& DeviceManager::getSessionAnalyticsManager() {
    return sessionAnalyticsManager;
}

PulseMonitorManager& DeviceManager::getPulseMonitorManager() {
    return pulseMonitorManager;
}

CommandProcessor& DeviceManager::getCommandProcessor() {
    return commandProcessor;
}

SessionManager& DeviceManager::getSessionManager() {
    return sessionManager;
}

bool DeviceManager::handleCommand(const String& command, CommandSource source) {
    Logger::infof("DeviceManager::handleCommand called - Command: '%s', Source: %d, State: %d",
                 command.c_str(), (int)source, (int)currentState);

    // Allow BLE commands even if WiFi/MQTT not ready, but require basic initialization
    if (currentState == DeviceState::INITIALIZING) {
        Logger::warning("Device still initializing - command ignored");
        return false;
    }

    if (currentState == DeviceState::ERROR) {
        // Allow RESET command even in error state for recovery
        String trimmedCommand = command;
        trimmedCommand.trim();

        if (trimmedCommand.equalsIgnoreCase("RESET") || trimmedCommand.equalsIgnoreCase("RECOVER")) {
            Logger::info("Recovery command received - attempting to exit error state");

            // Clear any critical errors that might be stuck
            ErrorHandler::clearErrors();
            Logger::info("Cleared error handler");

            // Force health check
            bool systemMonitorHealthy = systemMonitor.isSystemHealthy();
            bool errorHandlerHealthy = !ErrorHandler::hasCriticalErrors();
            Logger::infof("Post-reset health: SystemMonitor=%s, ErrorHandler=%s",
                         systemMonitorHealthy ? "OK" : "FAIL",
                         errorHandlerHealthy ? "OK" : "FAIL");

            setState(DeviceState::READY);
            return true;
        }

        Logger::warning("Device in error state - command ignored (send 'RESET' to recover)");
        return false;
    }

    Logger::infof("Received command from %s: %s",
                 commandProcessor.commandSourceToString(source).c_str(),
                 command.c_str());

    // Parse and validate command
    Logger::info("Parsing command...");
    Command cmd = commandProcessor.parseCommand(command, source);
    Logger::infof("Command parsed - Valid: %s, Type: %d, Code: %d",
                 cmd.isValid ? "YES" : "NO", (int)cmd.type, cmd.commandCode);

    if (!cmd.isValid) {
        Logger::warningf("Invalid command: %s", command.c_str());
        return false;
    }

    // Execute command immediately or queue it
    if (cmd.type == CommandType::MOVEMENT && servoController.isBusy()) {
        // Queue movement commands if servo is busy
        return commandProcessor.queueCommand(command, source);
    } else {
        // Execute immediately based on command type
        bool success = false;
        if (cmd.type == CommandType::MOVEMENT) {
            success = executeMovementCommand(cmd);
        } else if (cmd.type == CommandType::SYSTEM) {
            success = executeSystemCommand(cmd);
        } else {
            success = commandProcessor.executeCommand(cmd);
        }
        return success;
    }
}

void DeviceManager::processQueuedCommands() {
    // Process one queued command per update cycle to avoid blocking
    if (commandProcessor.hasQueuedCommands() && !servoController.isBusy()) {
        Command cmd = commandProcessor.getNextCommand();
        if (cmd.isValid) {
            if (cmd.type == CommandType::MOVEMENT) {
                executeMovementCommand(cmd);
            } else {
                commandProcessor.executeCommand(cmd);
            }
        }
    }
}

void DeviceManager::publishSystemStatus() {
    if (!mqttManager.isConnected()) return;

    // Collect current system metrics
    SystemMetrics metrics = systemMonitor.getSystemMetrics();

    // Update network status
    systemMonitor.updateNetworkStatus(
        wifiManager.isConnected(),
        mqttManager.isConnected(),
        bleManager.isConnected(),
        wifiManager.getSignalStrength(),
        wifiManager.getIPAddress()
    );

    // Publish via MQTT
    bool success = mqttManager.publishSystemStatus(
        systemMonitor.getHealthMessage(),
        FIRMWARE_VERSION,
        metrics.uptime / 1000,  // Convert to seconds
        metrics.freeHeap,
        wifiManager.isConnected(),
        bleManager.isConnected(),
        (int)servoController.getCurrentState(),
        wifiManager.getSignalStrength(),
        wifiManager.getIPAddress()
    );

    if (success) {
        Logger::debug("System status published");
    }

    // Publish enhanced system health data
    publishSystemHealthData();
}

void DeviceManager::logSystemSummary() {
    Logger::info("=== System Summary ===");
    Logger::infof("Device ID: %s", DEVICE_ID);
    Logger::infof("Firmware: %s", FIRMWARE_VERSION);
    Logger::infof("State: %d", (int)currentState);
    Logger::infof("WiFi: %s", wifiManager.isConnected() ? "Connected" : "Disconnected");
    Logger::infof("MQTT: %s", mqttManager.isConnected() ? "Connected" : "Disconnected");
    Logger::infof("BLE: %s", bleManager.isConnected() ? "Connected" : "Advertising");
    Logger::infof("Servos: %s", servoController.isBusy() ? "Busy" : "Ready");
    Logger::infof("Health: %s", systemMonitor.getHealthMessage().c_str());
    Logger::info("=====================");
}

void DeviceManager::setStatusReportInterval(unsigned long interval) {
    statusReportInterval = interval;
    Logger::infof("Status report interval set to %lu ms", interval);
}

void DeviceManager::setLogLevel(LogLevel level) {
    Logger::setLevel(level);
}

void DeviceManager::setStateChangeCallback(void (*callback)(DeviceState oldState, DeviceState newState)) {
    stateChangeCallback = callback;
}

bool DeviceManager::initializeFoundation() {
    Logger::info("Phase 1: Initializing Foundation...");

    // Initialize FreeRTOS infrastructure first
    Logger::info("Initializing FreeRTOS Manager with memory optimization...");
    if (!FreeRTOSManager::initialize()) {
        Logger::error("Failed to initialize FreeRTOS Manager");
        return false;
    }

    // Initialize I2C Manager for sensor communication
    Logger::info("Initializing I2C Manager...");
    if (!I2CManager::initialize()) {
        Logger::error("Failed to initialize I2C Manager");
        return false;
    }

    // Initialize core utilities
    TimeManager::initialize();
    ErrorHandler::initialize();

    Logger::info("Foundation initialization complete");
    logFreeRTOSStatus();  // Re-enabled with FreeRTOS Manager
    return true;
}

bool DeviceManager::initializeCommunication() {
    Logger::info("Phase 2: Initializing Communication...");

    // CRITICAL: Check heap before communication initialization
    size_t heapBefore = ESP.getFreeHeap();
    Logger::infof("Free heap before communication init: %u bytes", heapBefore);

    // Initialize WiFi
    Logger::info("Initializing WiFi Manager...");
    wifiManager.initialize();
    wifiManager.setConnectionCallback(onWiFiConnectionChange);

    // Initialize MQTT
    Logger::info("Initializing MQTT Manager...");
    mqttManager.initialize();
    mqttManager.setConnectionCallback(onMQTTConnectionChange);

    // CRITICAL: Initialize BLE with Static Memory (must be last for optimal memory layout)
    Logger::info("Initializing BLE Manager with Static Memory...");
    size_t heapBeforeBLE = ESP.getFreeHeap();
    Logger::infof("Free heap before BLE init: %u bytes", heapBeforeBLE);

    bleManager.initialize();
    bleManager.setConnectionCallback(onBLEConnectionChange);
    bleManager.setCommandCallback(onBLECommandReceived);

    // Verify BLE static memory health after initialization
    size_t heapAfterBLE = ESP.getFreeHeap();
    Logger::infof("Free heap after BLE init: %u bytes", heapAfterBLE);
    Logger::infof("Heap used by BLE: %u bytes", heapBeforeBLE - heapAfterBLE);

    Logger::info("Communication initialization complete");
    return true;
}

bool DeviceManager::initializeHardware() {
    Logger::info("Phase 3: Initializing Hardware...");

    // Initialize servo controller
    servoController.initialize();
    servoController.setMovementCompleteCallback(onServoMovementComplete);

    // Initialize system monitor
    systemMonitor.initialize();
    systemMonitor.setAlertCallback(onSystemAlert);
    systemMonitor.setStatusReportInterval(statusReportInterval);

    // SystemHealthManager removed - using SystemMonitor instead

    Logger::info("Hardware initialization complete");
    return true;
}

bool DeviceManager::initializeApplication() {
    Logger::info("Phase 4: Initializing Application...");

    // Initialize command processor
    commandProcessor.initialize();

    // Initialize session manager
    sessionManager.initialize();
    sessionManager.setSessionStartCallback(onSessionStart);
    sessionManager.setSessionEndCallback(onSessionEnd);
    sessionManager.setSessionStateChangeCallback(onSessionStateChange);

    // Initialize session analytics manager
    sessionAnalyticsManager.initialize();

    // Initialize pulse monitor manager
    Logger::info("About to initialize Pulse Monitor Manager...");
    pulseMonitorManager.initialize();
    pulseMonitorManager.setReadingCallback(onPulseReading);
    Logger::info("Pulse Monitor Manager initialization call completed");

    Logger::info("Application initialization complete");
    return true;
}

void DeviceManager::updateCommunication() {
    // WiFi Manager now runs in its own FreeRTOS task
    // wifiManager.update();  // Removed - handled by wifiManagerTask

    // MQTT Manager now runs in its own FreeRTOS tasks
    // mqttManager.update();  // Removed - handled by mqttPublisherTask and mqttSubscriberTask

    // BLE Manager now runs in its own FreeRTOS task
    // bleManager.update();  // Removed - handled by bleServerTask
}

void DeviceManager::updateHardware() {
    // ServoController now runs in its own FreeRTOS task
    // servoController.update();  // Removed - handled by servoTask
}

void DeviceManager::updateApplication() {
    // SessionManager now runs in its own FreeRTOS task (if needed)
    // sessionManager.update();  // Removed - minimal timeout checking can be event-driven

    // Check for new servo analytics and publish them
    if (servoController.hasNewAnalytics()) {
        publishServoAnalytics();
        servoController.clearNewAnalytics();
    }
}

void DeviceManager::updateMonitoring() {
    // SystemMonitor now event-driven - no polling needed
    // systemMonitor.update();  // Removed - monitoring is now task-based

    // Record loop performance for system monitoring
    unsigned long currentTime = millis();
    unsigned long currentLoopTime = currentTime - loopStartTime;
    systemMonitor.recordLoopTime(currentLoopTime);

    // Update performance tracking
    totalLoopTime += currentLoopTime;
    loopCount++;

    // Update next loop start time
    loopStartTime = currentTime;

    // Check for performance alerts
    if (currentLoopTime > MAX_LOOP_TIME) {
        Logger::warningf("Long loop time detected: %lu ms", currentLoopTime);
    }
}

bool DeviceManager::executeMovementCommand(const Command& command) {
    if (servoController.isBusy()) {
        Logger::warning("Servo controller busy");
        return false;
    }

    unsigned long startTime = millis();
    bool success = servoController.executeCommand(command.commandCode);
    unsigned long responseTime = millis() - startTime;

    if (success) {
        setState(DeviceState::RUNNING);
        publishMovementStatus(command, responseTime);

        // Record movement in session
        sessionManager.recordMovementCommand(command.rawCommand, success);
    }

    return success;
}

bool DeviceManager::executeSystemCommand(const Command& command) {
    String lower = command.rawCommand;
    lower.toLowerCase();

    if (lower == "status") {
        logSystemSummary();
        return true;
    } else if (lower == "restart") {
        Logger::warning("System restart requested");
        ESP.restart();
        return true;
    } else if (lower == "stats") {
        commandProcessor.logStatistics();
        logPerformanceMetrics();
        return true;
    } else if (lower == "end_session") {
        // Handle manual session end request
        if (sessionManager.isSessionActive()) {
            sessionManager.endSession("user_requested");
            Logger::info("Session ended by user request");
            return true;
        } else {
            Logger::warning("No active session to end");
            return false;
        }
    }

    return false;
}

void DeviceManager::setState(DeviceState newState) {
    if (newState != currentState) {
        DeviceState oldState = currentState;
        currentState = newState;

        Logger::infof("Device state changed: %d -> %d", (int)oldState, (int)newState);

        if (stateChangeCallback) {
            stateChangeCallback(oldState, newState);
        }
    }
}

void DeviceManager::checkSystemHealth() {
    // Detailed health analysis for debugging
    bool systemMonitorHealthy = systemMonitor.isSystemHealthy();
    bool errorHandlerHealthy = !ErrorHandler::hasCriticalErrors();
    bool overallHealthy = systemMonitorHealthy && errorHandlerHealthy;

    // Log detailed health status for debugging
    static unsigned long lastDetailedLog = 0;
    if (millis() - lastDetailedLog > 15000) {  // Every 15 seconds
        float memoryUsage = systemMonitor.getMemoryUsagePercent();
        SystemHealth health = systemMonitor.assessSystemHealth();

        Logger::infof("=== HEALTH DEBUG ===");
        SystemMetrics metrics = systemMonitor.getSystemMetrics();
        Logger::infof("Memory Usage: %.1f%% (%d/%d bytes)",
                     memoryUsage,
                     metrics.totalHeap - metrics.freeHeap,
                     metrics.totalHeap);
        Logger::infof("SystemMonitor Health: %s (Health Level: %d)",
                     systemMonitorHealthy ? "HEALTHY" : "UNHEALTHY", (int)health);
        Logger::infof("ErrorHandler Health: %s (Critical Errors: %s)",
                     errorHandlerHealthy ? "HEALTHY" : "UNHEALTHY",
                     ErrorHandler::hasCriticalErrors() ? "YES" : "NO");
        Logger::infof("Overall Health: %s", overallHealthy ? "HEALTHY" : "UNHEALTHY");
        Logger::infof("Current State: %d", (int)currentState);
        Logger::infof("==================");
        lastDetailedLog = millis();
    }

    if (!overallHealthy) {
        if (currentState != DeviceState::ERROR) {
            Logger::warningf("System health degraded - entering error state (Monitor: %s, Errors: %s)",
                           systemMonitorHealthy ? "OK" : "FAIL",
                           errorHandlerHealthy ? "OK" : "FAIL");
            setState(DeviceState::ERROR);
        }
    } else if (currentState == DeviceState::ERROR) {
        Logger::info("System health recovered - returning to ready state");
        setState(DeviceState::READY);
    }
}

void DeviceManager::publishMovementStatus(const Command& command, unsigned long responseTime) {
    if (mqttManager.isConnected()) {
        String sessionId = sessionManager.isSessionActive() ? sessionManager.getCurrentSessionId() : "";
        mqttManager.publishMovementCommand(
            command.rawCommand,
            responseTime,
            bleManager.isConnected(),
            sessionId
        );
    }
}

void DeviceManager::publishConnectionStatus() {
    if (mqttManager.isConnected()) {
        mqttManager.publishWiFiStatus(wifiManager.isConnected() ? "connected" : "disconnected");
        mqttManager.publishBLEStatus(bleManager.isConnected() ? "connected" : "advertising");
    }
}

void DeviceManager::publishSystemHealthData() {
    if (!mqttManager.isConnected()) return;

    // Get system metrics from SystemMonitor
    SystemMetrics metrics = systemMonitor.getSystemMetrics();

    // Publish system performance data
    bool success = mqttManager.publishPerformanceTiming(
        metrics.averageLoopTime,
        metrics.averageLoopTime,
        metrics.maxLoopTime
    );

    if (success) {
        Logger::debug("Published system performance data");
    }

    // Publish memory usage data
    success = mqttManager.publishPerformanceMemory(
        metrics.freeHeap,
        metrics.minFreeHeap,
        (float)((metrics.totalHeap - metrics.freeHeap) * 100.0 / metrics.totalHeap)
    );

    if (success) {
        Logger::debug("Published memory usage data");
    }


}

void DeviceManager::publishServoAnalytics() {
    if (!mqttManager.isConnected()) return;

    // Get the latest movement metrics from servo controller
    auto metrics = servoController.getLastMovementMetrics();

    // Get current session ID if active
    String sessionId = sessionManager.isSessionActive() ? sessionManager.getCurrentSessionId() : "";

    // Publish individual movement data
    bool success = mqttManager.publishMovementIndividual(
        metrics.servoIndex,
        metrics.startTime,
        metrics.duration,
        metrics.successful,
        metrics.startAngle,
        metrics.targetAngle,
        metrics.actualAngle,
        metrics.smoothness,
        metrics.movementType,
        sessionId
    );

    if (success) {
        Logger::debugf("Published servo analytics: Servo %d, Duration %lu ms, Quality %.2f",
                      metrics.servoIndex, metrics.duration, metrics.smoothness);

        // Also send to session analytics manager for processing
        sessionAnalyticsManager.processMovementData({
            metrics.servoIndex,
            metrics.startTime,
            metrics.duration,
            metrics.successful,
            metrics.startAngle,
            metrics.targetAngle,
            metrics.actualAngle,
            metrics.smoothness,
            metrics.movementType,
            sessionId
        });
    } else {
        Logger::warning("Failed to publish servo analytics");
    }
}

void DeviceManager::logPerformanceMetrics() {
    if (loopCount > 0) {
        unsigned long avgLoopTime = totalLoopTime / loopCount;
        Logger::infof("Performance Metrics:");
        Logger::infof("  Loop Count: %lu", loopCount);
        Logger::infof("  Average Loop Time: %lu ms", avgLoopTime);
        Logger::infof("  Max Loop Time: %lu ms", systemMonitor.getMaxLoopTime());
        Logger::infof("  Uptime: %s", systemMonitor.getUptimeString().c_str());
    }
}

// Static callback implementations
void DeviceManager::onWiFiConnectionChange(bool connected) {
    if (instance) {
        Logger::infof("WiFi connection changed: %s", connected ? "Connected" : "Disconnected");
        instance->publishConnectionStatus();

        if (connected) {
            TimeManager::syncWithNTP();
        }
    }
}

void DeviceManager::onMQTTConnectionChange(bool connected) {
    if (instance) {
        Logger::infof("MQTT connection changed: %s", connected ? "Connected" : "Disconnected");

        if (connected) {
            instance->publishConnectionStatus();
        }
    }
}

void DeviceManager::onBLEConnectionChange(bool connected) {
    if (instance) {
        static bool lastBLEState = false;
        static unsigned long lastBLEChange = 0;

        // Debounce BLE connection changes (only report if state actually changed and enough time passed)
        if (connected != lastBLEState && (millis() - lastBLEChange > 1000)) {
            Logger::infof("BLE connection changed: %s", connected ? "Connected" : "Disconnected");
            instance->publishConnectionStatus();

            // Handle session lifecycle based on BLE connection
            if (connected) {
                // Auto-start session on BLE connection
                if (!instance->sessionManager.isSessionActive()) {
                    instance->sessionManager.startSession(true);
                }
            } else {
                // Handle disconnection - mark session as interrupted if active
                if (instance->sessionManager.isSessionActive()) {
                    instance->sessionManager.endSession("ble_disconnection");
                }
            }

            lastBLEState = connected;
            lastBLEChange = millis();
        }
    }
}

void DeviceManager::onBLECommandReceived(const String& command) {
    Logger::infof("DeviceManager::onBLECommandReceived called with: '%s'", command.c_str());

    if (instance) {
        Logger::info("DeviceManager instance exists, calling handleCommand...");
        bool result = instance->handleCommand(command, CommandSource::BLE);
        Logger::infof("DeviceManager::handleCommand result: %s", result ? "SUCCESS" : "FAILED");
    } else {
        Logger::error("DeviceManager instance is null!");
    }
}

void DeviceManager::onServoMovementComplete(ServoState state, int cycles) {
    if (instance) {
        Logger::infof("Servo movement complete: State %d, Cycles %d", (int)state, cycles);

        // Record completed cycles in session
        instance->sessionManager.recordMovementComplete(cycles);

        if (instance->currentState == DeviceState::RUNNING) {
            instance->setState(DeviceState::READY);
        }

        // Servo movement completed successfully
        Logger::debug("Servo movement complete - no additional coordination needed");
    }
}

void DeviceManager::onSystemAlert(SystemHealth health, const String& message) {
    if (instance) {
        Logger::warningf("System Alert: %s", message.c_str());

        if (health == SystemHealth::CRITICAL) {
            instance->setState(DeviceState::ERROR);
        }
    }
}

// Session callback implementations
void DeviceManager::onSessionStart(const String& sessionId) {
    if (instance) {
        Logger::infof("Session started callback: %s", sessionId.c_str());

        // Process session start in analytics manager
        instance->sessionAnalyticsManager.processSessionStart(sessionId);

        // Start pulse monitoring session
        instance->pulseMonitorManager.startSession();

        // Publish session start to MQTT
        SessionManager& sm = instance->sessionManager;
        String sessionType = "";
        switch (sm.getCurrentType()) {
            case SessionType::SEQUENTIAL: sessionType = "sequential"; break;
            case SessionType::SIMULTANEOUS: sessionType = "simultaneous"; break;
            case SessionType::MIXED: sessionType = "mixed"; break;
            case SessionType::TEST_ONLY: sessionType = "test"; break;
            default: sessionType = "unknown"; break;
        }

        instance->mqttManager.publishSessionStart(sessionId, sessionType, instance->bleManager.isConnected());
    }
}

void DeviceManager::onSessionEnd(const String& sessionId, const SessionStats& stats) {
    if (instance) {
        Logger::infof("Session ended callback: %s", sessionId.c_str());
        Logger::infof("Session stats - Duration: %lu ms, Movements: %d, Cycles: %d, Success rate: %.1f%%",
                     stats.duration, stats.totalMovements, stats.completedCycles,
                     stats.totalMovements > 0 ? (float)stats.successfulMovements / stats.totalMovements * 100.0f : 0.0f);

        // Process session end in analytics manager
        instance->sessionAnalyticsManager.processSessionEnd(sessionId, stats.duration);

        // Generate final session analytics
        instance->sessionAnalyticsManager.generateSessionQuality(sessionId);
        instance->sessionAnalyticsManager.generateClinicalProgress(sessionId);

        // End pulse monitoring session
        instance->pulseMonitorManager.endSession();

        // Publish session end to MQTT
        String sessionType = "";
        switch (stats.detectedType) {
            case SessionType::SEQUENTIAL: sessionType = "sequential"; break;
            case SessionType::SIMULTANEOUS: sessionType = "simultaneous"; break;
            case SessionType::MIXED: sessionType = "mixed"; break;
            case SessionType::TEST_ONLY: sessionType = "test"; break;
            default: sessionType = "unknown"; break;
        }

        instance->mqttManager.publishSessionEnd(sessionId, sessionType, stats.endReason,
                                               stats.duration, stats.totalMovements,
                                               stats.successfulMovements, stats.completedCycles);
    }
}

void DeviceManager::onSessionStateChange(SessionState oldState, SessionState newState) {
    if (instance) {
        Logger::infof("Session state changed: %d -> %d", (int)oldState, (int)newState);
        // Future: Publish session state changes to MQTT
    }
}

void DeviceManager::onPulseReading(const HeartRateReading& reading) {
    if (instance && instance->mqttManager.isConnected()) {
        // Publish all readings to show sensor status and SpO2
        String qualityStr;
        switch (reading.quality) {
            case PulseQuality::GOOD: qualityStr = "good"; break;
            case PulseQuality::FAIR: qualityStr = "fair"; break;
            case PulseQuality::POOR: qualityStr = "poor"; break;
            case PulseQuality::NO_SIGNAL: qualityStr = "no_signal"; break;
            default: qualityStr = "unknown"; break;
        }

        String sessionId = instance->sessionManager.isSessionActive() ?
                          instance->sessionManager.getCurrentSessionId() : "";

        instance->mqttManager.publishHeartRate(
            reading.heartRate,
            reading.spO2,
            qualityStr,
            reading.fingerDetected, // Use actual finger detection
            sessionId
        );
    }
}

// Missing method implementations
void DeviceManager::onCommandComplete(const Command& command, bool success) {
    Logger::infof("Command completed: %s (Success: %s)",
                 command.rawCommand.c_str(), success ? "Yes" : "No");

    // Record command completion in session if active
    if (sessionManager.isSessionActive() && command.type == CommandType::MOVEMENT) {
        sessionManager.recordMovementCommand(command.rawCommand, success);
    }
}

void DeviceManager::onCommandError(const Command& command, const String& error) {
    Logger::errorf("Command error: %s - %s", command.rawCommand.c_str(), error.c_str());

    // Record failed command in session if active
    if (sessionManager.isSessionActive() && command.type == CommandType::MOVEMENT) {
        sessionManager.recordMovementCommand(command.rawCommand, false);
    }
}

void DeviceManager::handleSystemError() {
    Logger::error("System error detected - entering error state");
    setState(DeviceState::ERROR);

    // End active session if system error occurs
    if (sessionManager.isSessionActive()) {
        sessionManager.endSession("system_error");
    }
}

void DeviceManager::logComponentStatus() {
    Logger::info("=== Component Status ===");
    Logger::info("FreeRTOS Manager: Temporarily disabled");
    Logger::info("I2C Manager: Temporarily disabled");
    Logger::infof("WiFi Manager: %s (Task: %s)",
                 wifiManager.isConnected() ? "Connected" : "Disconnected",
                 wifiManager.isTaskRunning() ? "Running" : "Stopped");
    Logger::infof("MQTT Manager: %s (Tasks: %s)",
                 mqttManager.isConnected() ? "Connected" : "Disconnected",
                 mqttManager.areTasksRunning() ? "Running" : "Stopped");

    Logger::infof("BLE Manager: %s (Task: %s)",
                 bleManager.isConnected() ? "Connected" : "Advertising",
                 bleManager.isTaskRunning() ? "Running" : "Stopped");
    Logger::infof("Servo Controller: %s", servoController.isBusy() ? "Busy" : "Ready");
    Logger::infof("System Monitor: %s", systemMonitor.isSystemHealthy() ? "Healthy" : "Warning");
    // SystemHealthManager removed - using SystemMonitor instead
    Logger::infof("Session Analytics Manager: %s (Task: %s)",
                 "Ready",
                 sessionAnalyticsManager.isTaskRunning() ? "Running" : "Stopped");
    Logger::infof("Pulse Monitor Manager: %s (Task: %s)",
                 pulseMonitorManager.isSensorConnected() ? "Connected" : "Disconnected",
                 pulseMonitorManager.isTaskRunning() ? "Running" : "Stopped");
    Logger::infof("Session Manager: %s", sessionManager.isSessionActive() ? "Active Session" : "Idle");
    Logger::info("========================");
}

// Static FreeRTOS status methods
bool DeviceManager::isFreeRTOSReady() {
    // TODO: Re-enable when FreeRTOS Manager is active
    // return FreeRTOSManager::isInitialized() && I2CManager::isInitialized();
    return false;  // Temporarily disabled
}

void DeviceManager::logFreeRTOSStatus() {
    Logger::info("=== FreeRTOS System Status ===");
    Logger::infof("FreeRTOS Manager: %s", FreeRTOSManager::isInitialized() ? "ENABLED" : "DISABLED");
    Logger::infof("Free Heap: %u bytes", ESP.getFreeHeap());
    Logger::infof("Min Free Heap: %u bytes", ESP.getMinFreeHeap());
    Logger::infof("Task Count: %u", uxTaskGetNumberOfTasks());

    if (FreeRTOSManager::isInitialized()) {
        Logger::info("Memory-optimized configuration active for BLE compatibility");
        FreeRTOSManager::logSystemPerformance();
    }

    Logger::info("=============================");

    /*
    // TODO: Re-enable when FreeRTOS Manager is active
    Logger::infof("FreeRTOS Manager: %s", FreeRTOSManager::isInitialized() ? "Ready" : "Not Ready");
    Logger::infof("I2C Manager: %s", I2CManager::isInitialized() ? "Ready" : "Not Ready");
    Logger::infof("I2C Bus Health: %s", I2CManager::isBusHealthy() ? "Healthy" : "Issues Detected");
    Logger::infof("I2C Devices Found: %d", I2CManager::isInitialized() ? "Available" : "N/A");
    Logger::infof("Free Heap: %u bytes", FreeRTOSManager::getAvailableHeap());
    Logger::infof("Min Free Heap: %u bytes", FreeRTOSManager::getMinimumFreeHeap());

    if (FreeRTOSManager::isInitialized()) {
        FreeRTOSManager::logSystemPerformance();
    }
    */
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void DeviceManager::startTask() {
    if (taskRunning || taskHandle) return;

    BaseType_t result = xTaskCreatePinnedToCore(
        deviceManagerTask,
        "DeviceManager",
        4096,  // 4KB stack
        this,
        3,     // Priority 3 - coordination task
        &taskHandle,
        1      // Core 1 - Application CPU
    );

    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("DeviceManager task started on Core 1");
    } else {
        Logger::error("Failed to create DeviceManager task");
    }
}

void DeviceManager::stopTask() {
    if (!taskRunning || !taskHandle) return;

    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
    Logger::info("DeviceManager task stopped");
}

bool DeviceManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void DeviceManager::deviceManagerTask(void* parameter) {
    DeviceManager* manager = static_cast<DeviceManager*>(parameter);

    Logger::info("DeviceManager task started");

    while (manager->taskRunning) {
        // Run the main update cycle in FreeRTOS task context
        manager->update();

        // Task runs at 10Hz (100ms cycle) - appropriate for coordination
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    Logger::info("DeviceManager task ended");
    vTaskDelete(nullptr);
}
