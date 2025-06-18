#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>

enum class SystemHealth {
    EXCELLENT,
    GOOD,
    WARNING,
    CRITICAL,
    UNKNOWN
};

struct SystemMetrics {
    // Memory metrics
    size_t freeHeap;
    size_t totalHeap;
    size_t minFreeHeap;
    size_t maxAllocHeap;

    // System metrics
    unsigned long uptime;
    uint32_t cpuFrequency;
    float cpuTemperature;

    // Network metrics
    bool wifiConnected;
    bool mqttConnected;
    bool bleConnected;
    int wifiRssi;
    String ipAddress;

    // Performance metrics
    unsigned long loopCount;
    unsigned long averageLoopTime;
    unsigned long maxLoopTime;

    // Health assessment
    SystemHealth overallHealth;
    String healthMessage;
};

class SystemMonitor {
public:
    // Initialization and lifecycle
    void initialize();
    void update();

    // Metrics collection
    SystemMetrics getSystemMetrics();
    void collectMetrics();

    // Health assessment
    SystemHealth assessSystemHealth();
    String getHealthMessage();
    bool isSystemHealthy();

    // Memory monitoring
    size_t getFreeHeap();
    size_t getMinFreeHeap();
    float getMemoryUsagePercent();
    bool isLowMemory();

    // Performance monitoring
    void recordLoopTime(unsigned long loopTime);
    unsigned long getAverageLoopTime();
    unsigned long getMaxLoopTime();
    unsigned long getLoopCount();

    // System information
    unsigned long getUptime();
    String getUptimeString();
    uint32_t getCpuFrequency();
    String getChipModel();
    int getChipRevision();

    // Network status
    void updateNetworkStatus(bool wifi, bool mqtt, bool ble, int rssi, const String& ip);
    bool isNetworkHealthy();

    // Status reporting
    void publishStatusUpdate();
    void setStatusCallback(void (*callback)(const SystemMetrics& metrics));

    // Alerts and warnings
    void checkSystemAlerts();
    void setAlertCallback(void (*callback)(SystemHealth health, const String& message));

    // Configuration
    void setMemoryThreshold(size_t threshold);
    void setLoopTimeThreshold(unsigned long threshold);
    void setStatusReportInterval(unsigned long interval);

private:
    bool initialized;
    SystemMetrics currentMetrics;
    unsigned long lastStatusReport;
    unsigned long statusReportInterval;

    // Memory monitoring
    size_t memoryThreshold;
    size_t minFreeHeapRecorded;

    // Performance monitoring
    unsigned long loopTimeThreshold;
    unsigned long totalLoopTime;
    unsigned long loopCount;
    unsigned long maxLoopTimeRecorded;

    // Network status
    bool wifiConnected;
    bool mqttConnected;
    bool bleConnected;
    int wifiRssi;
    String ipAddress;

    // Callbacks
    void (*statusCallback)(const SystemMetrics& metrics);
    void (*alertCallback)(SystemHealth health, const String& message);

    // Internal methods
    void updateMemoryMetrics();
    void updateSystemMetrics();
    void updatePerformanceMetrics();
    void assessMemoryHealth();
    void assessPerformanceHealth();
    void assessNetworkHealth();

    // Logging and notifications
    void logSystemStatus();
    void logMemoryWarning();
    void logPerformanceWarning();
    void notifyAlert(SystemHealth health, const String& message);

    // Configuration constants
    static const size_t DEFAULT_MEMORY_THRESHOLD = 10000;  // 10KB
    static const unsigned long DEFAULT_LOOP_TIME_THRESHOLD = 100;  // 100ms
    static const unsigned long DEFAULT_STATUS_INTERVAL = 60000;  // 1 minute
    static constexpr float MEMORY_WARNING_PERCENT = 90.0f;   // Aligned with SystemHealthManager
    static constexpr float MEMORY_CRITICAL_PERCENT = 95.0f;  // Aligned with SystemHealthManager
};

#endif
