#ifndef FREERTOS_MANAGER_H
#define FREERTOS_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include "../config/Config.h"
#include "../utils/Logger.h"

// Forward declarations
class DeviceManager;

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// I2C Request Structure
struct I2CRequest {
    uint8_t deviceAddress;
    uint8_t* writeData;
    size_t writeLength;
    uint8_t* readBuffer;
    size_t readLength;
    SemaphoreHandle_t completionSemaphore;
    bool* successFlag;
    TickType_t timeout;
    uint32_t requestId;
};

// MQTT Message Structure
struct MQTTMessage {
    char topic[128];
    char payload[MAX_MQTT_MESSAGE_SIZE];
    bool retain;
    uint8_t qos;
    uint32_t timestamp;
    uint8_t priority;  // 0 = low, 255 = high
};

// System Alert Structure
struct SystemAlert {
    uint8_t level;        // 0=info, 1=warning, 2=error, 3=critical
    uint32_t errorCode;
    TaskHandle_t sourceTask;
    char description[64];
    uint32_t timestamp;
};

// Task Performance Metrics
struct TaskPerformanceMetrics {
    uint32_t executionTime;
    uint32_t maxExecutionTime;
    uint32_t missedDeadlines;
    uint32_t totalExecutions;
    uint32_t stackHighWaterMark;
    char taskName[16];
};

// =============================================================================
// FREERTOS MANAGER CLASS
// =============================================================================

class FreeRTOSManager {
public:
    // Lifecycle
    static bool initialize();
    static void shutdown();
    static bool isInitialized();

    // Queue Management
    static bool createQueues();
    static void destroyQueues();
    
    // Semaphore Management
    static bool createSemaphores();
    static void destroySemaphores();
    
    // Event Group Management
    static bool createEventGroups();
    static void destroyEventGroups();
    
    // Task Management
    static bool createTasks();
    static void destroyTasks();
    
    // Queue Access Methods
    static QueueHandle_t getPulseRawDataQueue();
    static QueueHandle_t getMotionRawDataQueue();
    static QueueHandle_t getPressureRawDataQueue();
    static QueueHandle_t getPulseProcessedQueue();
    static QueueHandle_t getMotionProcessedQueue();
    static QueueHandle_t getPressureProcessedQueue();
    static QueueHandle_t getServoCommandQueue();
    static QueueHandle_t getI2CRequestQueue();
    static QueueHandle_t getMQTTPublishQueue();
    static QueueHandle_t getSessionEventQueue();
    static QueueHandle_t getSystemAlertQueue();
    static QueueHandle_t getFusedDataQueue();
    static QueueHandle_t getMovementAnalyticsQueue();
    static QueueHandle_t getClinicalDataQueue();
    static QueueHandle_t getPerformanceMetricsQueue();

    // Semaphore Access Methods
    static SemaphoreHandle_t getI2CBusMutex();
    static SemaphoreHandle_t getServoControlMutex();
    static SemaphoreHandle_t getSessionDataMutex();
    static SemaphoreHandle_t getConfigMutex();
    static SemaphoreHandle_t getMQTTClientMutex();
    static SemaphoreHandle_t getPulseDataReady();
    static SemaphoreHandle_t getMotionDataReady();
    static SemaphoreHandle_t getSessionStarted();
    static SemaphoreHandle_t getEmergencyStop();
    static SemaphoreHandle_t getCalibrationComplete();

    // Semaphore Management Helper Methods
    static bool takeI2CBusMutex(TickType_t timeout = portMAX_DELAY);
    static void giveI2CBusMutex();
    static bool takeServoControlMutex(TickType_t timeout = portMAX_DELAY);
    static void giveServoControlMutex();
    static bool takeSessionDataMutex(TickType_t timeout = portMAX_DELAY);
    static void giveSessionDataMutex();
    static bool takeConfigMutex(TickType_t timeout = portMAX_DELAY);
    static void giveConfigMutex();

    // Event Group Access Methods
    static EventGroupHandle_t getSensorStatusEvents();
    static EventGroupHandle_t getSystemStateEvents();

    // Task Handle Access Methods
    static TaskHandle_t getWiFiManagerTask();
    static TaskHandle_t getMQTTPublisherTask();
    static TaskHandle_t getMQTTSubscriberTask();
    static TaskHandle_t getBLEServerTask();

    static TaskHandle_t getServoControlTask();
    static TaskHandle_t getI2CManagerTask();
    static TaskHandle_t getPulseMonitorTask();
    static TaskHandle_t getMotionSensorTask();
    static TaskHandle_t getPressureSensorTask();
    static TaskHandle_t getDataFusionTask();
    static TaskHandle_t getSessionAnalyticsTask();
    static TaskHandle_t getSystemHealthTask();

    // Task Handle Registration Methods (for external task creation)
    static void setServoControlTask(TaskHandle_t task);
    static void setI2CManagerTask(TaskHandle_t task);
    static void setPulseMonitorTask(TaskHandle_t task);
    static void setMotionSensorTask(TaskHandle_t task);
    static void setPressureSensorTask(TaskHandle_t task);
    static void setDataFusionTask(TaskHandle_t task);
    static void setSessionAnalyticsTask(TaskHandle_t task);
    static void setSystemHealthTask(TaskHandle_t task);

    // Performance Monitoring
    static void recordTaskPerformance(TaskHandle_t task, uint32_t executionTime);
    static TaskPerformanceMetrics getTaskMetrics(TaskHandle_t task);
    static void logSystemPerformance();

    // Error Handling
    static void reportSystemAlert(uint8_t level, uint32_t errorCode, const char* description);
    static bool handleSystemAlert(SystemAlert* alert);

    // Memory Management
    static void* allocateSensorData(size_t size);
    static void freeSensorData(void* ptr);
    static size_t getAvailableHeap();
    static size_t getMinimumFreeHeap();

    // System Health
    static bool checkSystemHealth();
    static void feedTaskWatchdog(TaskHandle_t task);
    static void registerTaskWatchdog(TaskHandle_t task, uint32_t timeoutMs);

private:
    // Initialization state
    static bool initialized;
    static bool queuesCreated;
    static bool semaphoresCreated;
    static bool eventGroupsCreated;
    static bool tasksCreated;

    // Queue handles
    static QueueHandle_t pulseRawDataQueue;
    static QueueHandle_t motionRawDataQueue;
    static QueueHandle_t pressureRawDataQueue;
    static QueueHandle_t pulseProcessedQueue;
    static QueueHandle_t motionProcessedQueue;
    static QueueHandle_t pressureProcessedQueue;
    static QueueHandle_t servoCommandQueue;
    static QueueHandle_t i2cRequestQueue;
    static QueueHandle_t mqttPublishQueue;
    static QueueHandle_t sessionEventQueue;
    static QueueHandle_t systemAlertQueue;
    static QueueHandle_t fusedDataQueue;
    static QueueHandle_t movementAnalyticsQueue;
    static QueueHandle_t clinicalDataQueue;
    static QueueHandle_t performanceMetricsQueue;

    // Semaphore handles
    static SemaphoreHandle_t i2cBusMutex;
    static SemaphoreHandle_t servoControlMutex;
    static SemaphoreHandle_t sessionDataMutex;
    static SemaphoreHandle_t configMutex;
    static SemaphoreHandle_t mqttClientMutex;
    static SemaphoreHandle_t pulseDataReady;
    static SemaphoreHandle_t motionDataReady;
    static SemaphoreHandle_t sessionStarted;
    static SemaphoreHandle_t emergencyStop;
    static SemaphoreHandle_t calibrationComplete;

    // Event group handles
    static EventGroupHandle_t sensorStatusEvents;
    static EventGroupHandle_t systemStateEvents;

    // Task handles
    static TaskHandle_t wifiManagerTask;
    static TaskHandle_t mqttPublisherTask;
    static TaskHandle_t mqttSubscriberTask;
    static TaskHandle_t bleServerTask;

    static TaskHandle_t servoControlTask;
    static TaskHandle_t i2cManagerTask;
    static TaskHandle_t pulseMonitorTask;
    static TaskHandle_t motionSensorTask;
    static TaskHandle_t pressureSensorTask;
    static TaskHandle_t dataFusionTask;
    static TaskHandle_t sessionAnalyticsTask;
    static TaskHandle_t systemHealthTask;

    // Performance tracking
    static TaskPerformanceMetrics taskMetrics[20];  // Support up to 20 tasks
    static uint8_t taskMetricsCount;

    // Memory pool for sensor data
    static uint8_t sensorDataPool[SENSOR_DATA_POOL_SIZE];
    static bool sensorDataPoolAllocated[SENSOR_DATA_POOL_ITEMS];
    static SemaphoreHandle_t sensorDataPoolMutex;

    // Internal helper methods
    static bool validateQueueCreation();
    static bool validateSemaphoreCreation();
    static bool validateEventGroupCreation();
    static void logInitializationStatus();
    static uint8_t findTaskMetricsIndex(TaskHandle_t task);
};

#endif // FREERTOS_MANAGER_H
