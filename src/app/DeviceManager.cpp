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

CommandProcessor& DeviceManager::getCommandProcessor() {
    return commandProcessor;
}

SessionManager& DeviceManager::getSessionManager() {
    return sessionManager;
}

bool DeviceManager::handleCommand(const String& command, CommandSource source) {
    // Allow BLE commands even if WiFi/MQTT not ready, but require basic initialization
    if (currentState == DeviceState::INITIALIZING) {
        Logger::warning("Device still initializing - command ignored");
        return false;
    }

    if (currentState == DeviceState::ERROR) {
        Logger::warning("Device in error state - command ignored");
        return false;
    }

    Logger::infof("Received command from %s: %s",
                 commandProcessor.commandSourceToString(source).c_str(),
                 command.c_str());

    // Parse and validate command
    Command cmd = commandProcessor.parseCommand(command, source);

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

    // Initialize core utilities
    TimeManager::initialize();
    ErrorHandler::initialize();

    Logger::info("Foundation initialization complete");
    return true;
}

bool DeviceManager::initializeCommunication() {
    Logger::info("Phase 2: Initializing Communication...");

    // Initialize WiFi
    wifiManager.initialize();
    wifiManager.setConnectionCallback(onWiFiConnectionChange);

    // Initialize MQTT
    mqttManager.initialize();
    mqttManager.setConnectionCallback(onMQTTConnectionChange);

    // Initialize BLE
    bleManager.initialize();
    bleManager.setConnectionCallback(onBLEConnectionChange);
    bleManager.setCommandCallback(onBLECommandReceived);

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

    Logger::info("Application initialization complete");
    return true;
}

void DeviceManager::updateCommunication() {
    wifiManager.update();
    mqttManager.update();
    bleManager.update();
}

void DeviceManager::updateHardware() {
    servoController.update();
}

void DeviceManager::updateApplication() {
    // Update command processor and handle any pending commands
    // Update session manager
    sessionManager.update();
}

void DeviceManager::updateMonitoring() {
    systemMonitor.update();
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
    if (!isHealthy()) {
        if (currentState != DeviceState::ERROR) {
            Logger::warning("System health degraded");
            setState(DeviceState::ERROR);
        }
    } else if (currentState == DeviceState::ERROR) {
        Logger::info("System health recovered");
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
    if (instance) {
        instance->handleCommand(command, CommandSource::BLE);
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
    Logger::infof("WiFi Manager: %s", wifiManager.isConnected() ? "Connected" : "Disconnected");
    Logger::infof("MQTT Manager: %s", mqttManager.isConnected() ? "Connected" : "Disconnected");
    Logger::infof("BLE Manager: %s", bleManager.isConnected() ? "Connected" : "Advertising");
    Logger::infof("Servo Controller: %s", servoController.isBusy() ? "Busy" : "Ready");
    Logger::infof("System Monitor: %s", systemMonitor.isSystemHealthy() ? "Healthy" : "Warning");
    Logger::infof("Session Manager: %s", sessionManager.isSessionActive() ? "Active Session" : "Idle");
    Logger::info("========================");
}
