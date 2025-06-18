#include "WiFiManager.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "../utils/TimeManager.h"
#include "../hardware/FreeRTOSManager.h"

void WiFiManager::initialize() {
    if (initialized) return;

    Logger::info("Initializing WiFi Manager...");

    currentStatus = WiFiStatus::DISCONNECTED;
    lastReconnectAttempt = 0;
    connectionStartTime = 0;
    connectedTime = 0;
    reconnectionCount = 0;
    connectionAttempts = 0;
    connectionCallback = nullptr;
    taskHandle = nullptr;
    taskRunning = false;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // We'll handle reconnection manually

    initialized = true;

    // Start initial connection attempt
    scanNetworks();
    attemptConnection();

    // Start the WiFi management task
    startTask();

    Logger::info("WiFi Manager initialized with FreeRTOS task");
}

void WiFiManager::update() {
    if (!initialized) return;
    
    handleConnectionEvents();
    updateConnectionStatus();
    
    // Handle reconnection logic
    if (currentStatus == WiFiStatus::DISCONNECTED || 
        currentStatus == WiFiStatus::CONNECTION_FAILED) {
        
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
            attemptConnection();
        }
    }
}

bool WiFiManager::isConnected() {
    return currentStatus == WiFiStatus::CONNECTED && WiFi.status() == WL_CONNECTED;
}

WiFiStatus WiFiManager::getStatus() {
    return currentStatus;
}

void WiFiManager::disconnect() {
    Logger::info("Disconnecting WiFi...");
    WiFi.disconnect();
    currentStatus = WiFiStatus::DISCONNECTED;
    notifyConnectionChange(false);
}

void WiFiManager::reconnect() {
    Logger::info("Manual WiFi reconnection requested");
    disconnect();
    delay(1000);
    attemptConnection();
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getMACAddress() {
    return WiFi.macAddress();
}

int WiFiManager::getSignalStrength() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return -100;  // Very weak signal indicator
}

String WiFiManager::getSSID() {
    if (isConnected()) {
        return WiFi.SSID();
    }
    return "";
}

void WiFiManager::scanNetworks() {
    Logger::info("Scanning for WiFi networks...");
    
    int networkCount = WiFi.scanNetworks();
    
    if (networkCount == 0) {
        Logger::warning("No WiFi networks found");
        return;
    }
    
    Logger::infof("Found %d networks:", networkCount);
    for (int i = 0; i < networkCount; i++) {
        String encryption = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted";
        Logger::infof("%d: %s (%d dBm) %s", 
                     i + 1, 
                     WiFi.SSID(i).c_str(), 
                     WiFi.RSSI(i), 
                     encryption.c_str());
    }
}

int WiFiManager::getNetworkCount() {
    return WiFi.scanComplete();
}

String WiFiManager::getNetworkSSID(int index) {
    if (index >= 0 && index < getNetworkCount()) {
        return WiFi.SSID(index);
    }
    return "";
}

int WiFiManager::getNetworkRSSI(int index) {
    if (index >= 0 && index < getNetworkCount()) {
        return WiFi.RSSI(index);
    }
    return -100;
}

bool WiFiManager::isNetworkEncrypted(int index) {
    if (index >= 0 && index < getNetworkCount()) {
        return WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    }
    return true;
}

void WiFiManager::setConnectionCallback(void (*callback)(bool connected)) {
    connectionCallback = callback;
}

unsigned long WiFiManager::getConnectionTime() {
    if (isConnected() && connectedTime > 0) {
        return millis() - connectedTime;
    }
    return 0;
}

int WiFiManager::getReconnectionCount() {
    return reconnectionCount;
}

unsigned long WiFiManager::getLastReconnectAttempt() {
    return lastReconnectAttempt;
}

void WiFiManager::attemptConnection() {
    if (currentStatus == WiFiStatus::CONNECTING) {
        return;  // Already attempting connection
    }
    
    Logger::infof("Attempting to connect to: %s", WIFI_SSID);
    
    currentStatus = WiFiStatus::CONNECTING;
    connectionStartTime = millis();
    lastReconnectAttempt = millis();
    connectionAttempts++;
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiManager::handleConnectionEvents() {
    static WiFiStatus lastStatus = WiFiStatus::DISCONNECTED;
    
    if (currentStatus != lastStatus) {
        logConnectionStatus();
        lastStatus = currentStatus;
    }
}

void WiFiManager::updateConnectionStatus() {
    wl_status_t wifiStatus = WiFi.status();
    
    switch (wifiStatus) {
        case WL_CONNECTED:
            if (currentStatus != WiFiStatus::CONNECTED) {
                currentStatus = WiFiStatus::CONNECTED;
                connectedTime = millis();
                
                Logger::info("WiFi connected!");
                logNetworkInfo();
                
                // Initialize time sync now that WiFi is connected
                TimeManager::syncWithNTP();
                
                notifyConnectionChange(true);
            }
            break;
            
        case WL_CONNECT_FAILED:
        case WL_CONNECTION_LOST:
        case WL_DISCONNECTED:
            if (currentStatus == WiFiStatus::CONNECTED) {
                Logger::warning("WiFi connection lost");
                notifyConnectionChange(false);
            }
            
            if (currentStatus == WiFiStatus::CONNECTING) {
                // Check for connection timeout
                if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
                    Logger::error("WiFi connection timeout");
                    currentStatus = WiFiStatus::CONNECTION_FAILED;
                    
                    if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
                        REPORT_ERROR(ErrorCode::WIFI_CONNECTION_FAILED, 
                                   "Max connection attempts reached");
                        connectionAttempts = 0;  // Reset for next cycle
                    }
                }
            } else {
                currentStatus = WiFiStatus::DISCONNECTED;
            }
            break;
            
        default:
            // Still connecting or other intermediate state
            break;
    }
}

void WiFiManager::logNetworkInfo() {
    Logger::infof("IP address: %s", getIPAddress().c_str());
    Logger::infof("Signal strength: %d dBm", getSignalStrength());
    Logger::infof("MAC address: %s", getMACAddress().c_str());
    Logger::infof("Connected to: %s", getSSID().c_str());
}

void WiFiManager::logConnectionStatus() {
    switch (currentStatus) {
        case WiFiStatus::DISCONNECTED:
            Logger::info("WiFi Status: Disconnected");
            break;
        case WiFiStatus::CONNECTING:
            Logger::info("WiFi Status: Connecting...");
            break;
        case WiFiStatus::CONNECTED:
            Logger::info("WiFi Status: Connected");
            break;
        case WiFiStatus::CONNECTION_FAILED:
            Logger::warning("WiFi Status: Connection Failed");
            break;
        case WiFiStatus::RECONNECTING:
            Logger::info("WiFi Status: Reconnecting...");
            break;
    }
}

void WiFiManager::notifyConnectionChange(bool connected) {
    if (connectionCallback) {
        connectionCallback(connected);
    }

    if (connected) {
        reconnectionCount++;
    }
}

void WiFiManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down WiFi Manager...");

    stopTask();
    disconnect();

    initialized = false;
    Logger::info("WiFi Manager shutdown complete");
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void WiFiManager::startTask() {
    if (taskRunning || taskHandle) return;

    BaseType_t result = xTaskCreatePinnedToCore(
        wifiManagerTask,
        "WiFiManager",
        TASK_STACK_WIFI_MANAGER,
        this,
        PRIORITY_WIFI_MANAGER,
        &taskHandle,
        CORE_PROTOCOL  // Core 0 for protocol tasks
    );

    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("WiFi Manager task started on Core 0");
    } else {
        Logger::error("Failed to create WiFi Manager task");
    }
}

void WiFiManager::stopTask() {
    if (!taskRunning || !taskHandle) return;

    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;

    Logger::info("WiFi Manager task stopped");
}

bool WiFiManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void WiFiManager::wifiManagerTask(void* parameter) {
    WiFiManager* manager = (WiFiManager*)parameter;
    Logger::info("WiFi Manager task started");

    while (manager->taskRunning) {
        // Handle connection events and status updates
        manager->handleConnectionEvents();
        manager->updateConnectionStatus();

        // Handle reconnection logic
        if (manager->currentStatus == WiFiStatus::DISCONNECTED ||
            manager->currentStatus == WiFiStatus::CONNECTION_FAILED) {

            unsigned long now = millis();
            if (now - manager->lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
                manager->attemptConnection();
            }
        }

        // Feed watchdog - now that FreeRTOS Manager is re-enabled
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());

        // Task runs every 1 second instead of continuous polling
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Logger::info("WiFi Manager task ended");
    vTaskDelete(nullptr);  // Delete self
}
