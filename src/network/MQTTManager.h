#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

enum class MQTTStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    RECONNECTING
};

class MQTTManager {
public:
    // Initialization and lifecycle
    void initialize();
    void update();

    // Connection management
    bool isConnected();
    MQTTStatus getStatus();
    void disconnect();
    void reconnect();

    // Publishing methods
    bool publishMovementCommand(const String& command, unsigned long responseTime, bool bleConnected, const String& sessionId = "");
    bool publishSystemStatus(const String& status, const String& firmwareVersion,
                           unsigned long uptime, size_t freeHeap, bool wifiConnected,
                           bool bleConnected, int currentState, int wifiRssi,
                           const String& ipAddress);
    bool publishWiFiStatus(const String& status);
    bool publishBLEStatus(const String& status);

    // Session management publishing
    bool publishSessionStart(const String& sessionId, const String& sessionType, bool bleConnected);
    bool publishSessionEnd(const String& sessionId, const String& sessionType, const String& endReason,
                          unsigned long duration, int totalMovements, int successfulMovements, int cycles);
    bool publishSessionProgress(const String& sessionId, int completedCycles, int totalCycles,
                               float progressPercent, int movementsCompleted);

    // Enhanced analytics publishing methods
    bool publishMovementIndividual(int servoIndex, unsigned long startTime, unsigned long duration,
                                  bool successful, int startAngle, int targetAngle, int actualAngle,
                                  float smoothness, const String& movementType, const String& sessionId = "");
    bool publishMovementQuality(const String& sessionId, float overallQuality,
                               float averageSmoothness, float successRate);
    bool publishPerformanceTiming(unsigned long loopTime, unsigned long averageLoopTime,
                                 unsigned long maxLoopTime);
    bool publishPerformanceMemory(size_t freeHeap, size_t minFreeHeap, float memoryUsagePercent);
    bool publishClinicalProgress(const String& sessionId, float progressScore,
                               const String& progressIndicators);
    bool publishClinicalQuality(const String& sessionId, float sessionQuality,
                              const String& qualityMetrics);

    // Generic publishing
    bool publish(const char* topic, const String& payload, bool retain = false);
    bool publishJSON(const char* topic, const JsonDocument& doc, bool retain = false);

    // Connection callbacks
    void setConnectionCallback(void (*callback)(bool connected));

    // Statistics
    unsigned long getConnectionTime();
    int getReconnectionCount();
    int getPublishCount();
    int getFailedPublishCount();
    unsigned long getLastPublishTime();

private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;

    MQTTStatus currentStatus;
    bool initialized;
    unsigned long lastReconnectAttempt;
    unsigned long connectionStartTime;
    unsigned long connectedTime;
    int reconnectionCount;
    int connectionAttempts;
    int publishCount;
    int failedPublishCount;
    unsigned long lastPublishTime;

    void (*connectionCallback)(bool connected);

    // Connection management
    void attemptConnection();
    void handleConnectionEvents();
    void updateConnectionStatus();

    // Publishing helpers
    JsonDocument createBaseDocument();
    bool publishWithRetry(const char* topic, const String& payload, bool retain = false);

    // Logging and notifications
    void logConnectionStatus();
    void notifyConnectionChange(bool connected);

    // Configuration
    static const int MAX_CONNECTION_ATTEMPTS = 3;
    static const unsigned long CONNECTION_TIMEOUT = 10000;  // 10 seconds
    static const int PUBLISH_RETRY_COUNT = 2;
};

#endif
