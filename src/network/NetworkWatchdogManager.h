#ifndef NETWORK_WATCHDOG_MANAGER_H
#define NETWORK_WATCHDOG_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../config/Config.h"

enum class NetworkHealth {
    EXCELLENT,
    GOOD,
    WARNING,
    CRITICAL,
    OFFLINE
};

enum class ConnectionType {
    WIFI,
    MQTT,
    BLE
};

struct ConnectionStatus {
    ConnectionType type;
    bool connected;
    unsigned long lastConnected;
    unsigned long lastAttempt;
    int failureCount;
    int recoveryAttempts;
    unsigned long responseTime;
    NetworkHealth health;
    String lastError;
};

struct NetworkMetrics {
    unsigned long totalUptime;
    unsigned long wifiUptime;
    unsigned long mqttUptime;
    unsigned long bleUptime;
    float wifiReliability;
    float mqttReliability;
    float bleReliability;
    int totalRecoveries;
    int successfulRecoveries;
    unsigned long averageResponseTime;
};

struct RecoveryAction {
    ConnectionType target;
    String action;
    unsigned long timestamp;
    bool successful;
    unsigned long duration;
};

class NetworkWatchdogManager {
public:
    // Initialization and lifecycle
    void initialize();
    void shutdown();
    
    // FreeRTOS task management
    void startTask();
    void stopTask();
    bool isTaskRunning();
    
    // Network monitoring
    NetworkHealth getOverallNetworkHealth();
    ConnectionStatus getConnectionStatus(ConnectionType type);
    NetworkMetrics getNetworkMetrics();
    bool isNetworkHealthy();
    
    // Connection monitoring
    void checkWiFiHealth();
    void checkMQTTHealth();
    void checkBLEHealth();
    void updateConnectionStatus(ConnectionType type, bool connected, unsigned long responseTime = 0);
    
    // Recovery management
    void triggerRecovery(ConnectionType type);
    void performWiFiRecovery();
    void performMQTTRecovery();
    void performBLERecovery();
    bool isRecoveryInProgress(ConnectionType type);
    
    // Performance monitoring
    void recordResponseTime(ConnectionType type, unsigned long responseTime);
    unsigned long getAverageResponseTime(ConnectionType type);
    float getConnectionReliability(ConnectionType type);
    
    // Alert system
    void reportNetworkAlert(ConnectionType type, const String& message);
    bool hasNewAlerts();
    String getLastAlert();
    void clearAlerts();
    
    // Statistics
    uint32_t getTotalRecoveries();
    uint32_t getSuccessfulRecoveries();
    float getRecoverySuccessRate();
    unsigned long getNetworkUptime();
    
    // Configuration
    void setRecoveryEnabled(bool enabled);
    void setMonitoringInterval(unsigned long interval);
    void setRecoveryThreshold(int failures);
    
    // Callbacks
    void setNetworkHealthCallback(void (*callback)(NetworkHealth health, const String& message));
    void setRecoveryCallback(void (*callback)(ConnectionType type, bool successful));

private:
    bool initialized;
    bool taskRunning;
    TaskHandle_t taskHandle;
    
    // Network state
    ConnectionStatus connections[3]; // WiFi, MQTT, BLE
    NetworkHealth overallHealth;
    bool recoveryEnabled;
    unsigned long monitoringInterval;
    int recoveryThreshold;
    
    // Performance tracking
    unsigned long startTime;
    unsigned long lastHealthCheck;
    unsigned long totalRecoveries;
    unsigned long successfulRecoveries;
    
    // Alert system
    String lastAlert;
    bool newAlertsAvailable;
    unsigned long alertCount;
    
    // Recovery tracking
    RecoveryAction recentRecoveries[10]; // Keep last 10 recovery actions
    int recoveryHistoryIndex;
    bool recoveryInProgress[3]; // Per connection type
    unsigned long recoveryStartTime[3];
    
    // Response time tracking
    struct ResponseTimeData {
        unsigned long totalTime;
        unsigned long count;
        unsigned long maxTime;
        unsigned long minTime;
    };
    ResponseTimeData responseTimes[3]; // Per connection type
    
    // Task function
    static void networkWatchdogTask(void* parameter);
    
    // Callbacks
    void (*networkHealthCallback)(NetworkHealth health, const String& message);
    void (*recoveryCallback)(ConnectionType type, bool successful);
    
    // Internal monitoring methods
    void performHealthChecks();
    void updateOverallHealth();
    void checkConnectionTimeouts();
    void evaluateRecoveryNeeds();
    void processRecoveryQueue();
    
    // Health assessment
    NetworkHealth assessConnectionHealth(ConnectionType type);
    NetworkHealth calculateOverallHealth();
    bool shouldTriggerRecovery(ConnectionType type);
    
    // Recovery implementation
    void executeRecovery(ConnectionType type);
    void recordRecoveryAction(ConnectionType type, const String& action, bool successful, unsigned long duration);
    void updateRecoveryMetrics(ConnectionType type, bool successful);
    
    // Performance calculations
    void updateConnectionMetrics(ConnectionType type);
    float calculateReliability(ConnectionType type);
    unsigned long calculateUptime(ConnectionType type);
    
    // Alert management
    void processNetworkAlerts();
    void generateHealthAlert(NetworkHealth health);
    void generateRecoveryAlert(ConnectionType type, bool successful);
    
    // Utility methods
    String connectionTypeToString(ConnectionType type);
    String networkHealthToString(NetworkHealth health);
    int getConnectionIndex(ConnectionType type);
    
    // Configuration
    static const unsigned long DEFAULT_MONITORING_INTERVAL = 10000;  // 10 seconds
    static const int DEFAULT_RECOVERY_THRESHOLD = 3;  // 3 failures
    static const unsigned long CONNECTION_TIMEOUT = 30000;  // 30 seconds
    static const unsigned long RECOVERY_TIMEOUT = 60000;  // 60 seconds
    static const unsigned long MAX_RESPONSE_TIME = 5000;  // 5 seconds
    static constexpr float RELIABILITY_EXCELLENT_THRESHOLD = 0.95f;  // 95%
    static constexpr float RELIABILITY_GOOD_THRESHOLD = 0.85f;  // 85%
    static constexpr float RELIABILITY_WARNING_THRESHOLD = 0.70f;  // 70%
};

#endif
