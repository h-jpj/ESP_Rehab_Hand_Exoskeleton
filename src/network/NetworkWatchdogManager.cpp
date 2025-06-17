#include "NetworkWatchdogManager.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"

void NetworkWatchdogManager::initialize() {
    if (initialized) return;
    
    Logger::info("Initializing Network Watchdog Manager...");
    
    taskRunning = false;
    taskHandle = nullptr;
    overallHealth = NetworkHealth::GOOD;
    recoveryEnabled = true;
    monitoringInterval = DEFAULT_MONITORING_INTERVAL;
    recoveryThreshold = DEFAULT_RECOVERY_THRESHOLD;
    startTime = millis();
    lastHealthCheck = 0;
    totalRecoveries = 0;
    successfulRecoveries = 0;
    lastAlert = "";
    newAlertsAvailable = false;
    alertCount = 0;
    recoveryHistoryIndex = 0;
    networkHealthCallback = nullptr;
    recoveryCallback = nullptr;
    
    // Initialize connection status
    for (int i = 0; i < 3; i++) {
        connections[i] = {};
        connections[i].type = (ConnectionType)i;
        connections[i].health = NetworkHealth::GOOD;
        recoveryInProgress[i] = false;
        recoveryStartTime[i] = 0;
        responseTimes[i] = {};
        responseTimes[i].minTime = UINT32_MAX;
    }
    
    // Initialize recovery history
    for (int i = 0; i < 10; i++) {
        recentRecoveries[i] = {};
    }
    
    initialized = true;
    
    // Start the network watchdog task
    startTask();
    
    Logger::info("Network Watchdog Manager initialized with FreeRTOS task");
}

void NetworkWatchdogManager::shutdown() {
    if (!initialized) return;
    
    Logger::info("Shutting down Network Watchdog Manager...");
    
    stopTask();
    
    initialized = false;
    Logger::info("Network Watchdog Manager shutdown complete");
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void NetworkWatchdogManager::startTask() {
    if (taskRunning || taskHandle) return;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        networkWatchdogTask,
        "NetworkWatchdog",
        TASK_STACK_NETWORK_WATCHDOG,
        this,
        PRIORITY_NETWORK_WATCHDOG,
        &taskHandle,
        CORE_PROTOCOL  // Core 0 for protocol tasks
    );
    
    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("Network Watchdog task started on Core 0");
    } else {
        Logger::error("Failed to create Network Watchdog task");
    }
}

void NetworkWatchdogManager::stopTask() {
    if (!taskRunning || !taskHandle) return;
    
    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
    
    Logger::info("Network Watchdog task stopped");
}

bool NetworkWatchdogManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void NetworkWatchdogManager::networkWatchdogTask(void* parameter) {
    NetworkWatchdogManager* manager = (NetworkWatchdogManager*)parameter;
    Logger::info("Network Watchdog task started");
    
    while (manager->taskRunning) {
        // Perform network health checks
        manager->performHealthChecks();
        
        // Update overall network health
        manager->updateOverallHealth();
        
        // Check for connection timeouts
        manager->checkConnectionTimeouts();
        
        // Evaluate recovery needs
        manager->evaluateRecoveryNeeds();
        
        // Process recovery queue
        manager->processRecoveryQueue();
        
        // Process network alerts
        manager->processNetworkAlerts();
        
        // Feed watchdog (when FreeRTOS Manager is re-enabled)
        // FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());
        
        // Network monitoring runs every 10 seconds
        vTaskDelay(pdMS_TO_TICKS(manager->monitoringInterval));
    }
    
    Logger::info("Network Watchdog task ended");
    vTaskDelete(nullptr);  // Delete self
}

// =============================================================================
// NETWORK MONITORING
// =============================================================================

NetworkHealth NetworkWatchdogManager::getOverallNetworkHealth() {
    return overallHealth;
}

ConnectionStatus NetworkWatchdogManager::getConnectionStatus(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index >= 0 && index < 3) {
        return connections[index];
    }
    return {};
}

NetworkMetrics NetworkWatchdogManager::getNetworkMetrics() {
    NetworkMetrics metrics;
    
    unsigned long currentTime = millis();
    metrics.totalUptime = currentTime - startTime;
    
    // Calculate individual uptimes and reliability
    metrics.wifiUptime = calculateUptime(ConnectionType::WIFI);
    metrics.mqttUptime = calculateUptime(ConnectionType::MQTT);
    metrics.bleUptime = calculateUptime(ConnectionType::BLE);
    
    metrics.wifiReliability = calculateReliability(ConnectionType::WIFI);
    metrics.mqttReliability = calculateReliability(ConnectionType::MQTT);
    metrics.bleReliability = calculateReliability(ConnectionType::BLE);
    
    metrics.totalRecoveries = totalRecoveries;
    metrics.successfulRecoveries = successfulRecoveries;
    
    // Calculate average response time across all connections
    unsigned long totalResponseTime = 0;
    unsigned long totalCount = 0;
    for (int i = 0; i < 3; i++) {
        totalResponseTime += responseTimes[i].totalTime;
        totalCount += responseTimes[i].count;
    }
    metrics.averageResponseTime = totalCount > 0 ? totalResponseTime / totalCount : 0;
    
    return metrics;
}

bool NetworkWatchdogManager::isNetworkHealthy() {
    return overallHealth == NetworkHealth::EXCELLENT || overallHealth == NetworkHealth::GOOD;
}

void NetworkWatchdogManager::checkWiFiHealth() {
    // Get actual WiFi status from WiFiManager
    // Note: This would need WiFiManager reference - for now use simulated data
    bool connected = true; // TODO: Get from WiFiManager::isConnected()
    unsigned long responseTime = 50; // TODO: Measure actual ping time

    updateConnectionStatus(ConnectionType::WIFI, connected, responseTime);
}

void NetworkWatchdogManager::checkMQTTHealth() {
    // This would integrate with MQTTManager to check actual MQTT status
    // For now, simulate health check
    bool connected = true; // Would get from MQTTManager
    unsigned long responseTime = 100; // Would measure actual publish time
    
    updateConnectionStatus(ConnectionType::MQTT, connected, responseTime);
}

void NetworkWatchdogManager::checkBLEHealth() {
    // This would integrate with BLEManager to check actual BLE status
    // For now, simulate health check
    bool connected = false; // Would get from BLEManager
    unsigned long responseTime = 20; // Would measure actual response time
    
    updateConnectionStatus(ConnectionType::BLE, connected, responseTime);
}

void NetworkWatchdogManager::updateConnectionStatus(ConnectionType type, bool connected, unsigned long responseTime) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return;
    
    ConnectionStatus& status = connections[index];
    unsigned long currentTime = millis();
    
    // Update connection status
    if (connected != status.connected) {
        if (connected) {
            status.lastConnected = currentTime;
            status.failureCount = 0; // Reset failure count on successful connection
            Logger::info("Network Watchdog: Connection restored");
        } else {
            status.failureCount++;
            status.lastError = "Connection lost";
            Logger::warning("Network Watchdog: Connection lost");
        }
        status.connected = connected;
    }
    
    // Update response time
    if (connected && responseTime > 0) {
        recordResponseTime(type, responseTime);
        status.responseTime = responseTime;
    }
    
    // Update health assessment
    status.health = assessConnectionHealth(type);
    
    // Update connection metrics
    updateConnectionMetrics(type);
}

// =============================================================================
// RECOVERY MANAGEMENT
// =============================================================================

void NetworkWatchdogManager::triggerRecovery(ConnectionType type) {
    if (!recoveryEnabled || isRecoveryInProgress(type)) return;
    
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return;
    
    Logger::info("Network Watchdog: Triggering recovery");
    
    recoveryInProgress[index] = true;
    recoveryStartTime[index] = millis();
    connections[index].recoveryAttempts++;
    
    executeRecovery(type);
}

void NetworkWatchdogManager::executeRecovery(ConnectionType type) {
    unsigned long recoveryStart = millis();
    bool successful = false;
    String action = "";
    
    switch (type) {
        case ConnectionType::WIFI:
            action = "WiFi restart";
            performWiFiRecovery();
            successful = true; // Would check actual result
            break;
            
        case ConnectionType::MQTT:
            action = "MQTT reconnect";
            performMQTTRecovery();
            successful = true; // Would check actual result
            break;
            
        case ConnectionType::BLE:
            action = "BLE restart";
            performBLERecovery();
            successful = true; // Would check actual result
            break;
    }
    
    unsigned long recoveryDuration = millis() - recoveryStart;
    
    // Record recovery action
    recordRecoveryAction(type, action, successful, recoveryDuration);
    
    // Update recovery metrics
    updateRecoveryMetrics(type, successful);
    
    // Clear recovery in progress flag
    int index = getConnectionIndex(type);
    if (index >= 0 && index < 3) {
        recoveryInProgress[index] = false;
    }
    
    // Notify via callback
    if (recoveryCallback) {
        recoveryCallback(type, successful);
    }
    
    Logger::info("Network Watchdog: Recovery completed");
}

void NetworkWatchdogManager::performWiFiRecovery() {
    // This would integrate with WiFiManager to perform actual WiFi recovery
    Logger::info("Network Watchdog: Performing WiFi recovery");
    // WiFiManager::restart() or similar
}

void NetworkWatchdogManager::performMQTTRecovery() {
    // This would integrate with MQTTManager to perform actual MQTT recovery
    Logger::info("Network Watchdog: Performing MQTT recovery");
    // MQTTManager::reconnect() or similar
}

void NetworkWatchdogManager::performBLERecovery() {
    // This would integrate with BLEManager to perform actual BLE recovery
    Logger::info("Network Watchdog: Performing BLE recovery");
    // BLEManager::restart() or similar
}

bool NetworkWatchdogManager::isRecoveryInProgress(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return false;

    return recoveryInProgress[index];
}

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

void NetworkWatchdogManager::recordResponseTime(ConnectionType type, unsigned long responseTime) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return;

    ResponseTimeData& data = responseTimes[index];
    data.totalTime += responseTime;
    data.count++;

    if (responseTime > data.maxTime) {
        data.maxTime = responseTime;
    }

    if (responseTime < data.minTime) {
        data.minTime = responseTime;
    }
}

unsigned long NetworkWatchdogManager::getAverageResponseTime(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return 0;

    ResponseTimeData& data = responseTimes[index];
    return data.count > 0 ? data.totalTime / data.count : 0;
}

float NetworkWatchdogManager::getConnectionReliability(ConnectionType type) {
    return calculateReliability(type);
}

// =============================================================================
// ALERT SYSTEM
// =============================================================================

void NetworkWatchdogManager::reportNetworkAlert(ConnectionType type, const String& message) {
    alertCount++;
    lastAlert = connectionTypeToString(type) + ": " + message;
    newAlertsAvailable = true;

    Logger::warningf("Network Alert: %s", lastAlert.c_str());

    // Notify via callback if set
    if (networkHealthCallback) {
        networkHealthCallback(overallHealth, lastAlert);
    }
}

bool NetworkWatchdogManager::hasNewAlerts() {
    return newAlertsAvailable;
}

String NetworkWatchdogManager::getLastAlert() {
    return lastAlert;
}

void NetworkWatchdogManager::clearAlerts() {
    newAlertsAvailable = false;
}

// =============================================================================
// STATISTICS
// =============================================================================

uint32_t NetworkWatchdogManager::getTotalRecoveries() {
    return totalRecoveries;
}

uint32_t NetworkWatchdogManager::getSuccessfulRecoveries() {
    return successfulRecoveries;
}

float NetworkWatchdogManager::getRecoverySuccessRate() {
    return totalRecoveries > 0 ? (float)successfulRecoveries / totalRecoveries : 0.0f;
}

unsigned long NetworkWatchdogManager::getNetworkUptime() {
    return millis() - startTime;
}

// =============================================================================
// CONFIGURATION
// =============================================================================

void NetworkWatchdogManager::setRecoveryEnabled(bool enabled) {
    recoveryEnabled = enabled;
    Logger::infof("Network Watchdog: Recovery %s", enabled ? "enabled" : "disabled");
}

void NetworkWatchdogManager::setMonitoringInterval(unsigned long interval) {
    monitoringInterval = interval;
    Logger::infof("Network Watchdog: Monitoring interval set to %lu ms", interval);
}

void NetworkWatchdogManager::setRecoveryThreshold(int failures) {
    recoveryThreshold = failures;
    Logger::infof("Network Watchdog: Recovery threshold set to %d failures", failures);
}

void NetworkWatchdogManager::setNetworkHealthCallback(void (*callback)(NetworkHealth health, const String& message)) {
    networkHealthCallback = callback;
}

void NetworkWatchdogManager::setRecoveryCallback(void (*callback)(ConnectionType type, bool successful)) {
    recoveryCallback = callback;
}

// =============================================================================
// INTERNAL METHODS
// =============================================================================

void NetworkWatchdogManager::performHealthChecks() {
    checkWiFiHealth();
    checkMQTTHealth();
    checkBLEHealth();
}

void NetworkWatchdogManager::updateOverallHealth() {
    overallHealth = calculateOverallHealth();
}

void NetworkWatchdogManager::checkConnectionTimeouts() {
    unsigned long currentTime = millis();

    for (int i = 0; i < 3; i++) {
        ConnectionStatus& status = connections[i];

        // Check for connection timeout
        if (status.connected &&
            currentTime - status.lastConnected > CONNECTION_TIMEOUT) {

            Logger::warningf("Network Watchdog: %s connection timeout",
                           connectionTypeToString(status.type).c_str());

            updateConnectionStatus(status.type, false);
        }

        // Check for recovery timeout
        if (recoveryInProgress[i] &&
            currentTime - recoveryStartTime[i] > RECOVERY_TIMEOUT) {

            Logger::warningf("Network Watchdog: %s recovery timeout",
                           connectionTypeToString(status.type).c_str());

            recoveryInProgress[i] = false;
        }
    }
}

void NetworkWatchdogManager::evaluateRecoveryNeeds() {
    for (int i = 0; i < 3; i++) {
        ConnectionStatus& status = connections[i];

        if (shouldTriggerRecovery(status.type)) {
            triggerRecovery(status.type);
        }
    }
}

void NetworkWatchdogManager::processRecoveryQueue() {
    // Process any pending recovery actions
    // This could be expanded to handle queued recovery requests
}

NetworkHealth NetworkWatchdogManager::assessConnectionHealth(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return NetworkHealth::OFFLINE;

    ConnectionStatus& status = connections[index];

    if (!status.connected) {
        return NetworkHealth::OFFLINE;
    }

    float reliability = calculateReliability(type);
    unsigned long avgResponseTime = getAverageResponseTime(type);

    if (reliability >= RELIABILITY_EXCELLENT_THRESHOLD && avgResponseTime < 1000) {
        return NetworkHealth::EXCELLENT;
    } else if (reliability >= RELIABILITY_GOOD_THRESHOLD && avgResponseTime < 2000) {
        return NetworkHealth::GOOD;
    } else if (reliability >= RELIABILITY_WARNING_THRESHOLD && avgResponseTime < 5000) {
        return NetworkHealth::WARNING;
    } else {
        return NetworkHealth::CRITICAL;
    }
}

NetworkHealth NetworkWatchdogManager::calculateOverallHealth() {
    NetworkHealth worstHealth = NetworkHealth::EXCELLENT;

    for (int i = 0; i < 3; i++) {
        if (connections[i].health > worstHealth) {
            worstHealth = connections[i].health;
        }
    }

    return worstHealth;
}

bool NetworkWatchdogManager::shouldTriggerRecovery(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return false;

    ConnectionStatus& status = connections[index];

    return !status.connected &&
           status.failureCount >= recoveryThreshold &&
           !isRecoveryInProgress(type);
}

void NetworkWatchdogManager::recordRecoveryAction(ConnectionType type, const String& action, bool successful, unsigned long duration) {
    RecoveryAction& recovery = recentRecoveries[recoveryHistoryIndex];
    recovery.target = type;
    recovery.action = action;
    recovery.timestamp = millis();
    recovery.successful = successful;
    recovery.duration = duration;

    recoveryHistoryIndex = (recoveryHistoryIndex + 1) % 10;
}

void NetworkWatchdogManager::updateRecoveryMetrics(ConnectionType type, bool successful) {
    totalRecoveries++;
    if (successful) {
        successfulRecoveries++;
    }
}

void NetworkWatchdogManager::updateConnectionMetrics(ConnectionType type) {
    // Update connection-specific metrics
    // This could include uptime calculations, reliability updates, etc.
}

float NetworkWatchdogManager::calculateReliability(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return 0.0f;

    // Simplified reliability calculation
    // In a real implementation, this would track connection history
    ConnectionStatus& status = connections[index];

    if (status.failureCount == 0) {
        return 1.0f;
    } else if (status.failureCount < 3) {
        return 0.8f;
    } else if (status.failureCount < 5) {
        return 0.6f;
    } else {
        return 0.3f;
    }
}

unsigned long NetworkWatchdogManager::calculateUptime(ConnectionType type) {
    int index = getConnectionIndex(type);
    if (index < 0 || index >= 3) return 0;

    // Simplified uptime calculation
    ConnectionStatus& status = connections[index];

    if (status.connected && status.lastConnected > 0) {
        return millis() - status.lastConnected;
    }

    return 0;
}

void NetworkWatchdogManager::processNetworkAlerts() {
    // Process any pending network alerts
    // This could integrate with SystemHealthManager for alert correlation
}

String NetworkWatchdogManager::connectionTypeToString(ConnectionType type) {
    switch (type) {
        case ConnectionType::WIFI: return "WiFi";
        case ConnectionType::MQTT: return "MQTT";
        case ConnectionType::BLE: return "BLE";
        default: return "Unknown";
    }
}

String NetworkWatchdogManager::networkHealthToString(NetworkHealth health) {
    switch (health) {
        case NetworkHealth::EXCELLENT: return "Excellent";
        case NetworkHealth::GOOD: return "Good";
        case NetworkHealth::WARNING: return "Warning";
        case NetworkHealth::CRITICAL: return "Critical";
        case NetworkHealth::OFFLINE: return "Offline";
        default: return "Unknown";
    }
}

int NetworkWatchdogManager::getConnectionIndex(ConnectionType type) {
    return (int)type;
}
