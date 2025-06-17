#include "FreeRTOSManager.h"

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

// Initialization state
bool FreeRTOSManager::initialized = false;
bool FreeRTOSManager::queuesCreated = false;
bool FreeRTOSManager::semaphoresCreated = false;
bool FreeRTOSManager::eventGroupsCreated = false;
bool FreeRTOSManager::tasksCreated = false;

// Queue handles
QueueHandle_t FreeRTOSManager::pulseRawDataQueue = nullptr;
QueueHandle_t FreeRTOSManager::motionRawDataQueue = nullptr;
QueueHandle_t FreeRTOSManager::pressureRawDataQueue = nullptr;
QueueHandle_t FreeRTOSManager::pulseProcessedQueue = nullptr;
QueueHandle_t FreeRTOSManager::motionProcessedQueue = nullptr;
QueueHandle_t FreeRTOSManager::pressureProcessedQueue = nullptr;
QueueHandle_t FreeRTOSManager::servoCommandQueue = nullptr;
QueueHandle_t FreeRTOSManager::i2cRequestQueue = nullptr;
QueueHandle_t FreeRTOSManager::mqttPublishQueue = nullptr;
QueueHandle_t FreeRTOSManager::sessionEventQueue = nullptr;
QueueHandle_t FreeRTOSManager::systemAlertQueue = nullptr;
QueueHandle_t FreeRTOSManager::fusedDataQueue = nullptr;
QueueHandle_t FreeRTOSManager::movementAnalyticsQueue = nullptr;
QueueHandle_t FreeRTOSManager::clinicalDataQueue = nullptr;
QueueHandle_t FreeRTOSManager::performanceMetricsQueue = nullptr;

// Semaphore handles
SemaphoreHandle_t FreeRTOSManager::i2cBusMutex = nullptr;
SemaphoreHandle_t FreeRTOSManager::servoControlMutex = nullptr;
SemaphoreHandle_t FreeRTOSManager::sessionDataMutex = nullptr;
SemaphoreHandle_t FreeRTOSManager::configMutex = nullptr;
SemaphoreHandle_t FreeRTOSManager::mqttClientMutex = nullptr;
SemaphoreHandle_t FreeRTOSManager::pulseDataReady = nullptr;
SemaphoreHandle_t FreeRTOSManager::motionDataReady = nullptr;
SemaphoreHandle_t FreeRTOSManager::sessionStarted = nullptr;
SemaphoreHandle_t FreeRTOSManager::emergencyStop = nullptr;
SemaphoreHandle_t FreeRTOSManager::calibrationComplete = nullptr;
SemaphoreHandle_t FreeRTOSManager::sensorDataPoolMutex = nullptr;

// Event group handles
EventGroupHandle_t FreeRTOSManager::sensorStatusEvents = nullptr;
EventGroupHandle_t FreeRTOSManager::systemStateEvents = nullptr;

// Task handles
TaskHandle_t FreeRTOSManager::wifiManagerTask = nullptr;
TaskHandle_t FreeRTOSManager::mqttPublisherTask = nullptr;
TaskHandle_t FreeRTOSManager::mqttSubscriberTask = nullptr;
TaskHandle_t FreeRTOSManager::bleServerTask = nullptr;
TaskHandle_t FreeRTOSManager::networkWatchdogTask = nullptr;
TaskHandle_t FreeRTOSManager::servoControlTask = nullptr;
TaskHandle_t FreeRTOSManager::i2cManagerTask = nullptr;
TaskHandle_t FreeRTOSManager::pulseMonitorTask = nullptr;
TaskHandle_t FreeRTOSManager::motionSensorTask = nullptr;
TaskHandle_t FreeRTOSManager::pressureSensorTask = nullptr;
TaskHandle_t FreeRTOSManager::dataFusionTask = nullptr;
TaskHandle_t FreeRTOSManager::sessionAnalyticsTask = nullptr;
TaskHandle_t FreeRTOSManager::systemHealthTask = nullptr;

// Performance tracking
TaskPerformanceMetrics FreeRTOSManager::taskMetrics[20];
uint8_t FreeRTOSManager::taskMetricsCount = 0;

// Memory pool
uint8_t FreeRTOSManager::sensorDataPool[SENSOR_DATA_POOL_SIZE];
bool FreeRTOSManager::sensorDataPoolAllocated[SENSOR_DATA_POOL_ITEMS];

// =============================================================================
// PUBLIC METHODS
// =============================================================================

bool FreeRTOSManager::initialize() {
    if (initialized) {
        Logger::warning("FreeRTOS Manager already initialized");
        return true;
    }

    Logger::info("Initializing FreeRTOS Manager...");

    // Initialize memory pool
    memset(sensorDataPool, 0, SENSOR_DATA_POOL_SIZE);
    memset(sensorDataPoolAllocated, false, SENSOR_DATA_POOL_ITEMS);
    memset(taskMetrics, 0, sizeof(taskMetrics));

    // Create FreeRTOS objects in order
    if (!createSemaphores()) {
        Logger::error("Failed to create semaphores");
        return false;
    }

    if (!createEventGroups()) {
        Logger::error("Failed to create event groups");
        return false;
    }

    if (!createQueues()) {
        Logger::error("Failed to create queues");
        return false;
    }

    // Note: Tasks will be created by individual managers
    // This allows for proper initialization order

    initialized = true;
    Logger::info("FreeRTOS Manager initialized successfully");
    logInitializationStatus();

    return true;
}

void FreeRTOSManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down FreeRTOS Manager...");

    destroyTasks();
    destroyQueues();
    destroySemaphores();
    destroyEventGroups();

    initialized = false;
    queuesCreated = false;
    semaphoresCreated = false;
    eventGroupsCreated = false;
    tasksCreated = false;

    Logger::info("FreeRTOS Manager shutdown complete");
}

bool FreeRTOSManager::isInitialized() {
    return initialized;
}

bool FreeRTOSManager::createQueues() {
    if (queuesCreated) return true;

    Logger::info("Creating FreeRTOS queues...");

    // High-frequency sensor data queues
    pulseRawDataQueue = xQueueCreate(QUEUE_SIZE_PULSE_RAW, sizeof(uint32_t));  // Placeholder size
    motionRawDataQueue = xQueueCreate(QUEUE_SIZE_MOTION_RAW, sizeof(uint32_t));
    pressureRawDataQueue = xQueueCreate(QUEUE_SIZE_PRESSURE_RAW, sizeof(uint32_t));

    // Processed sensor data queues
    pulseProcessedQueue = xQueueCreate(QUEUE_SIZE_PULSE_PROCESSED, sizeof(uint32_t));
    motionProcessedQueue = xQueueCreate(QUEUE_SIZE_MOTION_PROCESSED, sizeof(uint32_t));
    pressureProcessedQueue = xQueueCreate(QUEUE_SIZE_PRESSURE_PROCESSED, sizeof(uint32_t));

    // Command and control queues
    servoCommandQueue = xQueueCreate(QUEUE_SIZE_SERVO_COMMANDS, sizeof(uint32_t));
    i2cRequestQueue = xQueueCreate(QUEUE_SIZE_I2C_REQUESTS, sizeof(I2CRequest));
    mqttPublishQueue = xQueueCreate(QUEUE_SIZE_MQTT_PUBLISH, sizeof(MQTTMessage));
    sessionEventQueue = xQueueCreate(QUEUE_SIZE_SESSION_EVENTS, sizeof(uint32_t));
    systemAlertQueue = xQueueCreate(QUEUE_SIZE_SYSTEM_ALERTS, sizeof(SystemAlert));

    // Analytics and fusion queues
    fusedDataQueue = xQueueCreate(QUEUE_SIZE_FUSED_DATA, sizeof(uint32_t));
    movementAnalyticsQueue = xQueueCreate(QUEUE_SIZE_MOVEMENT_ANALYTICS, sizeof(uint32_t));
    clinicalDataQueue = xQueueCreate(QUEUE_SIZE_CLINICAL_DATA, sizeof(uint32_t));
    performanceMetricsQueue = xQueueCreate(QUEUE_SIZE_PERFORMANCE_METRICS, sizeof(TaskPerformanceMetrics));

    if (!validateQueueCreation()) {
        Logger::error("Queue creation validation failed");
        return false;
    }

    queuesCreated = true;
    Logger::info("All queues created successfully");
    return true;
}

bool FreeRTOSManager::createSemaphores() {
    if (semaphoresCreated) return true;

    Logger::info("Creating FreeRTOS semaphores...");

    // Mutexes for resource protection
    i2cBusMutex = xSemaphoreCreateMutex();
    servoControlMutex = xSemaphoreCreateMutex();
    sessionDataMutex = xSemaphoreCreateMutex();
    configMutex = xSemaphoreCreateMutex();
    mqttClientMutex = xSemaphoreCreateMutex();
    sensorDataPoolMutex = xSemaphoreCreateMutex();

    // Binary semaphores for event signaling
    pulseDataReady = xSemaphoreCreateBinary();
    motionDataReady = xSemaphoreCreateBinary();
    sessionStarted = xSemaphoreCreateBinary();
    emergencyStop = xSemaphoreCreateBinary();
    calibrationComplete = xSemaphoreCreateBinary();

    if (!validateSemaphoreCreation()) {
        Logger::error("Semaphore creation validation failed");
        return false;
    }

    semaphoresCreated = true;
    Logger::info("All semaphores created successfully");
    return true;
}

bool FreeRTOSManager::createEventGroups() {
    if (eventGroupsCreated) return true;

    Logger::info("Creating FreeRTOS event groups...");

    sensorStatusEvents = xEventGroupCreate();
    systemStateEvents = xEventGroupCreate();

    if (!validateEventGroupCreation()) {
        Logger::error("Event group creation validation failed");
        return false;
    }

    eventGroupsCreated = true;
    Logger::info("All event groups created successfully");
    return true;
}

void FreeRTOSManager::destroyQueues() {
    if (!queuesCreated) return;

    Logger::info("Destroying FreeRTOS queues...");

    // Delete all queues
    if (pulseRawDataQueue) { vQueueDelete(pulseRawDataQueue); pulseRawDataQueue = nullptr; }
    if (motionRawDataQueue) { vQueueDelete(motionRawDataQueue); motionRawDataQueue = nullptr; }
    if (pressureRawDataQueue) { vQueueDelete(pressureRawDataQueue); pressureRawDataQueue = nullptr; }
    if (pulseProcessedQueue) { vQueueDelete(pulseProcessedQueue); pulseProcessedQueue = nullptr; }
    if (motionProcessedQueue) { vQueueDelete(motionProcessedQueue); motionProcessedQueue = nullptr; }
    if (pressureProcessedQueue) { vQueueDelete(pressureProcessedQueue); pressureProcessedQueue = nullptr; }
    if (servoCommandQueue) { vQueueDelete(servoCommandQueue); servoCommandQueue = nullptr; }
    if (i2cRequestQueue) { vQueueDelete(i2cRequestQueue); i2cRequestQueue = nullptr; }
    if (mqttPublishQueue) { vQueueDelete(mqttPublishQueue); mqttPublishQueue = nullptr; }
    if (sessionEventQueue) { vQueueDelete(sessionEventQueue); sessionEventQueue = nullptr; }
    if (systemAlertQueue) { vQueueDelete(systemAlertQueue); systemAlertQueue = nullptr; }
    if (fusedDataQueue) { vQueueDelete(fusedDataQueue); fusedDataQueue = nullptr; }
    if (movementAnalyticsQueue) { vQueueDelete(movementAnalyticsQueue); movementAnalyticsQueue = nullptr; }
    if (clinicalDataQueue) { vQueueDelete(clinicalDataQueue); clinicalDataQueue = nullptr; }
    if (performanceMetricsQueue) { vQueueDelete(performanceMetricsQueue); performanceMetricsQueue = nullptr; }

    queuesCreated = false;
    Logger::info("All queues destroyed");
}

void FreeRTOSManager::destroySemaphores() {
    if (!semaphoresCreated) return;

    Logger::info("Destroying FreeRTOS semaphores...");

    // Delete all semaphores
    if (i2cBusMutex) { vSemaphoreDelete(i2cBusMutex); i2cBusMutex = nullptr; }
    if (servoControlMutex) { vSemaphoreDelete(servoControlMutex); servoControlMutex = nullptr; }
    if (sessionDataMutex) { vSemaphoreDelete(sessionDataMutex); sessionDataMutex = nullptr; }
    if (configMutex) { vSemaphoreDelete(configMutex); configMutex = nullptr; }
    if (mqttClientMutex) { vSemaphoreDelete(mqttClientMutex); mqttClientMutex = nullptr; }
    if (sensorDataPoolMutex) { vSemaphoreDelete(sensorDataPoolMutex); sensorDataPoolMutex = nullptr; }
    if (pulseDataReady) { vSemaphoreDelete(pulseDataReady); pulseDataReady = nullptr; }
    if (motionDataReady) { vSemaphoreDelete(motionDataReady); motionDataReady = nullptr; }
    if (sessionStarted) { vSemaphoreDelete(sessionStarted); sessionStarted = nullptr; }
    if (emergencyStop) { vSemaphoreDelete(emergencyStop); emergencyStop = nullptr; }
    if (calibrationComplete) { vSemaphoreDelete(calibrationComplete); calibrationComplete = nullptr; }

    semaphoresCreated = false;
    Logger::info("All semaphores destroyed");
}

void FreeRTOSManager::destroyEventGroups() {
    if (!eventGroupsCreated) return;

    Logger::info("Destroying FreeRTOS event groups...");

    if (sensorStatusEvents) { vEventGroupDelete(sensorStatusEvents); sensorStatusEvents = nullptr; }
    if (systemStateEvents) { vEventGroupDelete(systemStateEvents); systemStateEvents = nullptr; }

    eventGroupsCreated = false;
    Logger::info("All event groups destroyed");
}

// =============================================================================
// QUEUE ACCESS METHODS
// =============================================================================

QueueHandle_t FreeRTOSManager::getPulseRawDataQueue() { return pulseRawDataQueue; }
QueueHandle_t FreeRTOSManager::getMotionRawDataQueue() { return motionRawDataQueue; }
QueueHandle_t FreeRTOSManager::getPressureRawDataQueue() { return pressureRawDataQueue; }
QueueHandle_t FreeRTOSManager::getPulseProcessedQueue() { return pulseProcessedQueue; }
QueueHandle_t FreeRTOSManager::getMotionProcessedQueue() { return motionProcessedQueue; }
QueueHandle_t FreeRTOSManager::getPressureProcessedQueue() { return pressureProcessedQueue; }
QueueHandle_t FreeRTOSManager::getServoCommandQueue() { return servoCommandQueue; }
QueueHandle_t FreeRTOSManager::getI2CRequestQueue() { return i2cRequestQueue; }
QueueHandle_t FreeRTOSManager::getMQTTPublishQueue() { return mqttPublishQueue; }
QueueHandle_t FreeRTOSManager::getSessionEventQueue() { return sessionEventQueue; }
QueueHandle_t FreeRTOSManager::getSystemAlertQueue() { return systemAlertQueue; }
QueueHandle_t FreeRTOSManager::getFusedDataQueue() { return fusedDataQueue; }
QueueHandle_t FreeRTOSManager::getMovementAnalyticsQueue() { return movementAnalyticsQueue; }
QueueHandle_t FreeRTOSManager::getClinicalDataQueue() { return clinicalDataQueue; }
QueueHandle_t FreeRTOSManager::getPerformanceMetricsQueue() { return performanceMetricsQueue; }

// =============================================================================
// SEMAPHORE ACCESS METHODS
// =============================================================================

SemaphoreHandle_t FreeRTOSManager::getI2CBusMutex() { return i2cBusMutex; }
SemaphoreHandle_t FreeRTOSManager::getServoControlMutex() { return servoControlMutex; }
SemaphoreHandle_t FreeRTOSManager::getSessionDataMutex() { return sessionDataMutex; }
SemaphoreHandle_t FreeRTOSManager::getConfigMutex() { return configMutex; }
SemaphoreHandle_t FreeRTOSManager::getMQTTClientMutex() { return mqttClientMutex; }
SemaphoreHandle_t FreeRTOSManager::getPulseDataReady() { return pulseDataReady; }
SemaphoreHandle_t FreeRTOSManager::getMotionDataReady() { return motionDataReady; }
SemaphoreHandle_t FreeRTOSManager::getSessionStarted() { return sessionStarted; }
SemaphoreHandle_t FreeRTOSManager::getEmergencyStop() { return emergencyStop; }
SemaphoreHandle_t FreeRTOSManager::getCalibrationComplete() { return calibrationComplete; }

// =============================================================================
// EVENT GROUP ACCESS METHODS
// =============================================================================

EventGroupHandle_t FreeRTOSManager::getSensorStatusEvents() { return sensorStatusEvents; }
EventGroupHandle_t FreeRTOSManager::getSystemStateEvents() { return systemStateEvents; }

// =============================================================================
// TASK HANDLE ACCESS METHODS
// =============================================================================

TaskHandle_t FreeRTOSManager::getWiFiManagerTask() { return wifiManagerTask; }
TaskHandle_t FreeRTOSManager::getMQTTPublisherTask() { return mqttPublisherTask; }
TaskHandle_t FreeRTOSManager::getMQTTSubscriberTask() { return mqttSubscriberTask; }
TaskHandle_t FreeRTOSManager::getBLEServerTask() { return bleServerTask; }
TaskHandle_t FreeRTOSManager::getNetworkWatchdogTask() { return networkWatchdogTask; }
TaskHandle_t FreeRTOSManager::getServoControlTask() { return servoControlTask; }
TaskHandle_t FreeRTOSManager::getI2CManagerTask() { return i2cManagerTask; }
TaskHandle_t FreeRTOSManager::getPulseMonitorTask() { return pulseMonitorTask; }
TaskHandle_t FreeRTOSManager::getMotionSensorTask() { return motionSensorTask; }
TaskHandle_t FreeRTOSManager::getPressureSensorTask() { return pressureSensorTask; }
TaskHandle_t FreeRTOSManager::getDataFusionTask() { return dataFusionTask; }
TaskHandle_t FreeRTOSManager::getSessionAnalyticsTask() { return sessionAnalyticsTask; }
TaskHandle_t FreeRTOSManager::getSystemHealthTask() { return systemHealthTask; }

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

void FreeRTOSManager::recordTaskPerformance(TaskHandle_t task, uint32_t executionTime) {
    if (!task) return;

    uint8_t index = findTaskMetricsIndex(task);
    if (index >= 20) return;  // Array bounds check

    TaskPerformanceMetrics* metrics = &taskMetrics[index];
    metrics->executionTime = executionTime;
    metrics->totalExecutions++;

    if (executionTime > metrics->maxExecutionTime) {
        metrics->maxExecutionTime = executionTime;
    }

    if (executionTime > MAX_TASK_EXECUTION_TIME_MS) {
        metrics->missedDeadlines++;
        Logger::warningf("Task %s exceeded execution time: %lu ms",
                        pcTaskGetName(task), executionTime);
    }

    // Update stack high water mark
    metrics->stackHighWaterMark = uxTaskGetStackHighWaterMark(task);
}

TaskPerformanceMetrics FreeRTOSManager::getTaskMetrics(TaskHandle_t task) {
    if (!task) return {};

    uint8_t index = findTaskMetricsIndex(task);
    if (index >= 20) return {};

    return taskMetrics[index];
}

void FreeRTOSManager::logSystemPerformance() {
    Logger::info("=== FreeRTOS System Performance ===");
    Logger::infof("Free heap: %u bytes", xPortGetFreeHeapSize());
    Logger::infof("Minimum free heap: %u bytes", xPortGetMinimumEverFreeHeapSize());
    Logger::infof("Task count: %u", uxTaskGetNumberOfTasks());

    for (uint8_t i = 0; i < taskMetricsCount; i++) {
        TaskPerformanceMetrics* metrics = &taskMetrics[i];
        if (metrics->totalExecutions > 0) {
            Logger::infof("Task %s: Exec=%lu, Max=%lu ms, Missed=%lu, Stack=%lu",
                         metrics->taskName,
                         metrics->totalExecutions,
                         metrics->maxExecutionTime,
                         metrics->missedDeadlines,
                         metrics->stackHighWaterMark);
        }
    }
    Logger::info("================================");
}

// =============================================================================
// ERROR HANDLING
// =============================================================================

void FreeRTOSManager::reportSystemAlert(uint8_t level, uint32_t errorCode, const char* description) {
    if (!systemAlertQueue) return;

    SystemAlert alert;
    alert.level = level;
    alert.errorCode = errorCode;
    alert.sourceTask = xTaskGetCurrentTaskHandle();
    alert.timestamp = millis();
    strncpy(alert.description, description, sizeof(alert.description) - 1);
    alert.description[sizeof(alert.description) - 1] = '\0';

    BaseType_t result = xQueueSend(systemAlertQueue, &alert, pdMS_TO_TICKS(100));
    if (result != pdTRUE) {
        Logger::error("Failed to queue system alert - queue full");
    }
}

bool FreeRTOSManager::handleSystemAlert(SystemAlert* alert) {
    if (!alert || !systemAlertQueue) return false;

    BaseType_t result = xQueueReceive(systemAlertQueue, alert, 0);
    return (result == pdTRUE);
}

// =============================================================================
// MEMORY MANAGEMENT
// =============================================================================

void* FreeRTOSManager::allocateSensorData(size_t size) {
    if (!sensorDataPoolMutex || size > (SENSOR_DATA_POOL_SIZE / SENSOR_DATA_POOL_ITEMS)) {
        return nullptr;
    }

    if (xSemaphoreTake(sensorDataPoolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (size_t i = 0; i < SENSOR_DATA_POOL_ITEMS; i++) {
            if (!sensorDataPoolAllocated[i]) {
                sensorDataPoolAllocated[i] = true;
                xSemaphoreGive(sensorDataPoolMutex);
                return &sensorDataPool[i * (SENSOR_DATA_POOL_SIZE / SENSOR_DATA_POOL_ITEMS)];
            }
        }
        xSemaphoreGive(sensorDataPoolMutex);
    }

    return nullptr;  // Pool exhausted
}

void FreeRTOSManager::freeSensorData(void* ptr) {
    if (!ptr || !sensorDataPoolMutex) return;

    if (xSemaphoreTake(sensorDataPoolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        size_t itemSize = SENSOR_DATA_POOL_SIZE / SENSOR_DATA_POOL_ITEMS;
        size_t index = ((uint8_t*)ptr - sensorDataPool) / itemSize;

        if (index < SENSOR_DATA_POOL_ITEMS) {
            sensorDataPoolAllocated[index] = false;
        }

        xSemaphoreGive(sensorDataPoolMutex);
    }
}

size_t FreeRTOSManager::getAvailableHeap() {
    return xPortGetFreeHeapSize();
}

size_t FreeRTOSManager::getMinimumFreeHeap() {
    return xPortGetMinimumEverFreeHeapSize();
}

// =============================================================================
// SYSTEM HEALTH
// =============================================================================

bool FreeRTOSManager::checkSystemHealth() {
    bool healthy = true;

    // Check heap memory
    size_t freeHeap = xPortGetFreeHeapSize();
    if (freeHeap < MIN_FREE_HEAP) {
        Logger::warningf("Low heap memory: %u bytes", freeHeap);
        healthy = false;
    }

    // Check queue usage
    if (systemAlertQueue && uxQueueMessagesWaiting(systemAlertQueue) > (QUEUE_SIZE_SYSTEM_ALERTS * 0.8)) {
        Logger::warning("System alert queue nearly full");
        healthy = false;
    }

    // Check for missed deadlines
    for (uint8_t i = 0; i < taskMetricsCount; i++) {
        if (taskMetrics[i].missedDeadlines > 0) {
            Logger::warningf("Task %s has %lu missed deadlines",
                           taskMetrics[i].taskName, taskMetrics[i].missedDeadlines);
            healthy = false;
        }
    }

    return healthy;
}

void FreeRTOSManager::feedTaskWatchdog(TaskHandle_t task) {
    // Implementation depends on ESP32 watchdog configuration
    // For now, just log the watchdog feed
    if (task) {
        Logger::debugf("Watchdog fed by task: %s", pcTaskGetName(task));
    }
}

void FreeRTOSManager::registerTaskWatchdog(TaskHandle_t task, uint32_t timeoutMs) {
    // Implementation depends on ESP32 watchdog configuration
    if (task) {
        Logger::infof("Watchdog registered for task: %s (timeout: %lu ms)",
                     pcTaskGetName(task), timeoutMs);
    }
}

// =============================================================================
// TASK MANAGEMENT
// =============================================================================

bool FreeRTOSManager::createTasks() {
    // Note: Individual task creation will be handled by their respective managers
    // This method is reserved for future system-wide task creation if needed
    tasksCreated = true;
    return true;
}

void FreeRTOSManager::destroyTasks() {
    if (!tasksCreated) return;

    Logger::info("Destroying FreeRTOS tasks...");

    // Delete all tasks if they exist
    if (wifiManagerTask) { vTaskDelete(wifiManagerTask); wifiManagerTask = nullptr; }
    if (mqttPublisherTask) { vTaskDelete(mqttPublisherTask); mqttPublisherTask = nullptr; }
    if (mqttSubscriberTask) { vTaskDelete(mqttSubscriberTask); mqttSubscriberTask = nullptr; }
    if (bleServerTask) { vTaskDelete(bleServerTask); bleServerTask = nullptr; }
    if (networkWatchdogTask) { vTaskDelete(networkWatchdogTask); networkWatchdogTask = nullptr; }
    if (servoControlTask) { vTaskDelete(servoControlTask); servoControlTask = nullptr; }
    if (i2cManagerTask) { vTaskDelete(i2cManagerTask); i2cManagerTask = nullptr; }
    if (pulseMonitorTask) { vTaskDelete(pulseMonitorTask); pulseMonitorTask = nullptr; }
    if (motionSensorTask) { vTaskDelete(motionSensorTask); motionSensorTask = nullptr; }
    if (pressureSensorTask) { vTaskDelete(pressureSensorTask); pressureSensorTask = nullptr; }
    if (dataFusionTask) { vTaskDelete(dataFusionTask); dataFusionTask = nullptr; }
    if (sessionAnalyticsTask) { vTaskDelete(sessionAnalyticsTask); sessionAnalyticsTask = nullptr; }
    if (systemHealthTask) { vTaskDelete(systemHealthTask); systemHealthTask = nullptr; }

    tasksCreated = false;
    Logger::info("All tasks destroyed");
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

bool FreeRTOSManager::validateQueueCreation() {
    bool valid = true;

    if (!pulseRawDataQueue) { Logger::error("Failed to create pulseRawDataQueue"); valid = false; }
    if (!motionRawDataQueue) { Logger::error("Failed to create motionRawDataQueue"); valid = false; }
    if (!pressureRawDataQueue) { Logger::error("Failed to create pressureRawDataQueue"); valid = false; }
    if (!pulseProcessedQueue) { Logger::error("Failed to create pulseProcessedQueue"); valid = false; }
    if (!motionProcessedQueue) { Logger::error("Failed to create motionProcessedQueue"); valid = false; }
    if (!pressureProcessedQueue) { Logger::error("Failed to create pressureProcessedQueue"); valid = false; }
    if (!servoCommandQueue) { Logger::error("Failed to create servoCommandQueue"); valid = false; }
    if (!i2cRequestQueue) { Logger::error("Failed to create i2cRequestQueue"); valid = false; }
    if (!mqttPublishQueue) { Logger::error("Failed to create mqttPublishQueue"); valid = false; }
    if (!sessionEventQueue) { Logger::error("Failed to create sessionEventQueue"); valid = false; }
    if (!systemAlertQueue) { Logger::error("Failed to create systemAlertQueue"); valid = false; }
    if (!fusedDataQueue) { Logger::error("Failed to create fusedDataQueue"); valid = false; }
    if (!movementAnalyticsQueue) { Logger::error("Failed to create movementAnalyticsQueue"); valid = false; }
    if (!clinicalDataQueue) { Logger::error("Failed to create clinicalDataQueue"); valid = false; }
    if (!performanceMetricsQueue) { Logger::error("Failed to create performanceMetricsQueue"); valid = false; }

    return valid;
}

bool FreeRTOSManager::validateSemaphoreCreation() {
    bool valid = true;

    if (!i2cBusMutex) { Logger::error("Failed to create i2cBusMutex"); valid = false; }
    if (!servoControlMutex) { Logger::error("Failed to create servoControlMutex"); valid = false; }
    if (!sessionDataMutex) { Logger::error("Failed to create sessionDataMutex"); valid = false; }
    if (!configMutex) { Logger::error("Failed to create configMutex"); valid = false; }
    if (!mqttClientMutex) { Logger::error("Failed to create mqttClientMutex"); valid = false; }
    if (!sensorDataPoolMutex) { Logger::error("Failed to create sensorDataPoolMutex"); valid = false; }
    if (!pulseDataReady) { Logger::error("Failed to create pulseDataReady"); valid = false; }
    if (!motionDataReady) { Logger::error("Failed to create motionDataReady"); valid = false; }
    if (!sessionStarted) { Logger::error("Failed to create sessionStarted"); valid = false; }
    if (!emergencyStop) { Logger::error("Failed to create emergencyStop"); valid = false; }
    if (!calibrationComplete) { Logger::error("Failed to create calibrationComplete"); valid = false; }

    return valid;
}

bool FreeRTOSManager::validateEventGroupCreation() {
    bool valid = true;

    if (!sensorStatusEvents) { Logger::error("Failed to create sensorStatusEvents"); valid = false; }
    if (!systemStateEvents) { Logger::error("Failed to create systemStateEvents"); valid = false; }

    return valid;
}

void FreeRTOSManager::logInitializationStatus() {
    Logger::info("=== FreeRTOS Manager Status ===");
    Logger::infof("Queues created: %s", queuesCreated ? "Yes" : "No");
    Logger::infof("Semaphores created: %s", semaphoresCreated ? "Yes" : "No");
    Logger::infof("Event groups created: %s", eventGroupsCreated ? "Yes" : "No");
    Logger::infof("Tasks created: %s", tasksCreated ? "Yes" : "No");
    Logger::infof("Free heap: %u bytes", xPortGetFreeHeapSize());
    Logger::infof("Task count: %u", uxTaskGetNumberOfTasks());
    Logger::info("==============================");
}

uint8_t FreeRTOSManager::findTaskMetricsIndex(TaskHandle_t task) {
    if (!task) return 255;  // Invalid index

    // Search for existing entry
    for (uint8_t i = 0; i < taskMetricsCount; i++) {
        if (strcmp(taskMetrics[i].taskName, pcTaskGetName(task)) == 0) {
            return i;
        }
    }

    // Create new entry if space available
    if (taskMetricsCount < 20) {
        uint8_t index = taskMetricsCount++;
        strncpy(taskMetrics[index].taskName, pcTaskGetName(task), sizeof(taskMetrics[index].taskName) - 1);
        taskMetrics[index].taskName[sizeof(taskMetrics[index].taskName) - 1] = '\0';
        return index;
    }

    return 255;  // No space available
}
