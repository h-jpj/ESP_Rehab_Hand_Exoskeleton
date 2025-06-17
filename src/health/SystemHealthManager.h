#ifndef SYSTEM_HEALTH_MANAGER_H
#define SYSTEM_HEALTH_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../config/Config.h"

enum class HealthStatus {
    EXCELLENT,
    GOOD,
    WARNING,
    CRITICAL,
    ERROR
};

struct MemoryMetrics {
    size_t totalHeap;
    size_t freeHeap;
    size_t minFreeHeap;
    size_t largestFreeBlock;
    float usagePercent;
    float fragmentationPercent;
};

struct TaskMetrics {
    TaskHandle_t handle;
    String name;
    UBaseType_t priority;
    uint32_t stackHighWaterMark;
    uint32_t runtime;
    float cpuUsage;
    eTaskState state;
};

struct SystemHealthReport {
    HealthStatus overallHealth;
    MemoryMetrics memory;
    uint32_t uptime;
    uint32_t totalTasks;
    uint32_t runningTasks;
    float averageCpuUsage;
    uint32_t systemAlerts;
    String lastAlert;
};

class SystemHealthManager {
public:
    // Initialization and lifecycle
    void initialize();
    void shutdown();
    
    // FreeRTOS task management
    void startTask();
    void stopTask();
    bool isTaskRunning();
    
    // Health monitoring
    HealthStatus getOverallHealth();
    MemoryMetrics getMemoryMetrics();
    SystemHealthReport getHealthReport();
    
    // Task monitoring
    uint32_t getTaskCount();
    TaskMetrics getTaskMetrics(TaskHandle_t taskHandle);
    void logTaskPerformance();
    
    // Alert system
    void reportAlert(const String& alert, HealthStatus severity);
    bool hasNewAlerts();
    String getLastAlert();
    void clearAlerts();
    
    // Performance monitoring
    void recordLoopTime(uint32_t loopTime);
    uint32_t getAverageLoopTime();
    uint32_t getMaxLoopTime();
    
    // System statistics
    uint32_t getUptime();
    float getCpuUsage();
    bool isSystemHealthy();
    
    // Callbacks
    void setHealthCallback(void (*callback)(HealthStatus status, const String& message));

private:
    bool initialized;
    HealthStatus currentHealth;
    uint32_t startTime;
    uint32_t alertCount;
    String lastAlert;
    bool newAlertsAvailable;
    
    // Performance tracking
    uint32_t totalLoopTime;
    uint32_t loopCount;
    uint32_t maxLoopTime;
    uint32_t lastHealthCheck;
    
    // Memory tracking
    size_t lastFreeHeap;
    size_t minFreeHeapEver;
    uint32_t memoryLeakCount;
    
    // Task management
    TaskHandle_t taskHandle;
    bool taskRunning;
    static void systemHealthTask(void* parameter);
    
    // Callback
    void (*healthCallback)(HealthStatus status, const String& message);
    
    // Internal monitoring methods
    void updateMemoryMetrics();
    void updateTaskMetrics();
    void checkSystemHealth();
    void detectMemoryLeaks();
    void monitorTaskPerformance();
    
    // Health assessment
    HealthStatus assessOverallHealth();
    HealthStatus assessMemoryHealth();
    HealthStatus assessTaskHealth();
    
    // Alert management
    void processSystemAlerts();
    void logHealthReport();
    
    // Configuration
    static const uint32_t HEALTH_CHECK_INTERVAL = 5000;  // 5 seconds
    static const uint32_t MEMORY_LEAK_THRESHOLD = 1024;  // 1KB
    static constexpr float MEMORY_WARNING_THRESHOLD = 80.0f; // 80%
    static constexpr float MEMORY_CRITICAL_THRESHOLD = 90.0f; // 90%
    static const uint32_t MAX_LOOP_TIME_WARNING = 100;   // 100ms
    static const uint32_t MAX_LOOP_TIME_CRITICAL = 500;  // 500ms
};

#endif
