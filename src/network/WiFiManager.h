#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../config/Config.h"

enum class WiFiStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    RECONNECTING
};

class WiFiManager {
public:
    // Initialization and lifecycle
    void initialize();
    void update();  // Legacy method - will be deprecated
    void shutdown();

    // FreeRTOS task management
    void startTask();
    void stopTask();
    bool isTaskRunning();
    
    // Connection management
    bool isConnected();
    WiFiStatus getStatus();
    void disconnect();
    void reconnect();
    
    // Network information
    String getIPAddress();
    String getMACAddress();
    int getSignalStrength();
    String getSSID();
    
    // Network scanning
    void scanNetworks();
    int getNetworkCount();
    String getNetworkSSID(int index);
    int getNetworkRSSI(int index);
    bool isNetworkEncrypted(int index);
    
    // Status callbacks
    void setConnectionCallback(void (*callback)(bool connected));
    
    // Statistics
    unsigned long getConnectionTime();
    int getReconnectionCount();
    unsigned long getLastReconnectAttempt();

private:
    WiFiStatus currentStatus;
    bool initialized;
    unsigned long lastReconnectAttempt;
    unsigned long connectionStartTime;
    unsigned long connectedTime;
    int reconnectionCount;
    int connectionAttempts;

    void (*connectionCallback)(bool connected);

    // FreeRTOS task management
    TaskHandle_t taskHandle;
    bool taskRunning;
    static void wifiManagerTask(void* parameter);
    
    // Connection management
    void attemptConnection();
    void handleConnectionEvents();
    void updateConnectionStatus();
    
    // Logging and notifications
    void logNetworkInfo();
    void logConnectionStatus();
    void notifyConnectionChange(bool connected);
    
    // Configuration
    static const int MAX_CONNECTION_ATTEMPTS = 5;
    static const unsigned long CONNECTION_TIMEOUT = 10000;  // 10 seconds
};

#endif
