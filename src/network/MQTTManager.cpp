#include "MQTTManager.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "../utils/TimeManager.h"
#include "../hardware/FreeRTOSManager.h"

void MQTTManager::initialize() {
    if (initialized) return;

    Logger::info("Initializing MQTT Manager...");

    currentStatus = MQTTStatus::DISCONNECTED;
    lastReconnectAttempt = 0;
    connectionStartTime = 0;
    connectedTime = 0;
    reconnectionCount = 0;
    connectionAttempts = 0;
    publishCount = 0;
    failedPublishCount = 0;
    lastPublishTime = 0;
    connectionCallback = nullptr;
    publisherTaskHandle = nullptr;
    subscriberTaskHandle = nullptr;
    tasksRunning = false;

    mqttClient.setClient(wifiClient);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient.setKeepAlive(60);  // 60 second keep-alive
    mqttClient.setSocketTimeout(30);  // 30 second socket timeout

    initialized = true;

    // Start MQTT tasks
    startTasks();

    Logger::infof("MQTT Manager initialized with FreeRTOS tasks: %s:%d (buffer: %d bytes)",
                 MQTT_SERVER, MQTT_PORT, MQTT_BUFFER_SIZE);
}

void MQTTManager::update() {
    if (!initialized) return;

    // Maintain MQTT connection
    if (mqttClient.connected()) {
        mqttClient.loop();
    }

    handleConnectionEvents();
    updateConnectionStatus();

    // Handle reconnection logic
    if ((currentStatus == MQTTStatus::DISCONNECTED ||
         currentStatus == MQTTStatus::CONNECTION_FAILED) &&
        WiFi.status() == WL_CONNECTED) {

        unsigned long now = millis();
        if (now - lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
            attemptConnection();
        }
    }
}

bool MQTTManager::isConnected() {
    return currentStatus == MQTTStatus::CONNECTED && mqttClient.connected();
}

MQTTStatus MQTTManager::getStatus() {
    return currentStatus;
}

void MQTTManager::disconnect() {
    Logger::info("Disconnecting MQTT...");
    mqttClient.disconnect();
    currentStatus = MQTTStatus::DISCONNECTED;
    notifyConnectionChange(false);
}

void MQTTManager::reconnect() {
    Logger::info("Manual MQTT reconnection requested");
    disconnect();
    delay(1000);
    attemptConnection();
}

bool MQTTManager::publishMovementCommand(const String& command, unsigned long responseTime, bool bleConnected, const String& sessionId) {
    if (!isConnected()) {
        REPORT_WARNING(ErrorCode::MQTT_CONNECTION_FAILED, "Cannot publish - MQTT not connected");
        return false;
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "movement_command";

    JsonObject data = doc["data"].to<JsonObject>();
    data["command"] = command;
    data["response_time_ms"] = responseTime;
    data["ble_connected"] = bleConnected;
    if (sessionId.length() > 0) {
        data["session_id"] = sessionId;
    }

    bool success = publishJSON(TOPIC_MOVEMENT_COMMAND, doc);
    if (success) {
        Logger::infof("Published movement command: %s (Session: %s)", command.c_str(), sessionId.c_str());
    }

    return success;
}

bool MQTTManager::publishSystemStatus(const String& status, const String& firmwareVersion,
                                    unsigned long uptime, size_t freeHeap, bool wifiConnected,
                                    bool bleConnected, int currentState, int wifiRssi,
                                    const String& ipAddress) {
    if (!isConnected()) {
        return false;  // Don't log warning for system status - too frequent
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "system_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = status;
    data["firmware_version"] = firmwareVersion;
    data["uptime_seconds"] = uptime;
    data["free_heap"] = freeHeap;
    data["wifi_connected"] = wifiConnected;
    data["ble_connected"] = bleConnected;
    data["current_state"] = currentState;
    data["wifi_rssi"] = wifiRssi;
    data["ip_address"] = ipAddress;

    bool success = publishJSON(TOPIC_SYSTEM_STATUS, doc);
    if (success) {
        Logger::debug("Published system status");
    }

    return success;
}

bool MQTTManager::publishWiFiStatus(const String& status) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "wifi_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = status;

    bool success = publishJSON(TOPIC_CONNECTION_WIFI, doc);
    if (success) {
        Logger::infof("Published WiFi status: %s", status.c_str());
    }

    return success;
}

bool MQTTManager::publishBLEStatus(const String& status) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "ble_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = status;

    bool success = publishJSON(TOPIC_CONNECTION_BLE, doc);
    if (success) {
        Logger::infof("Published BLE status: %s", status.c_str());
    }

    return success;
}

// Session management publishing methods
bool MQTTManager::publishSessionStart(const String& sessionId, const String& sessionType, bool bleConnected) {
    if (!isConnected()) {
        REPORT_WARNING(ErrorCode::MQTT_CONNECTION_FAILED, "Cannot publish session start - MQTT not connected");
        return false;
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "session_start";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["session_type"] = sessionType;
    data["ble_connected"] = bleConnected;
    data["auto_started"] = true;

    bool success = publishJSON(TOPIC_SESSION_START, doc);
    if (success) {
        Logger::infof("Published session start: %s", sessionId.c_str());
    } else {
        Logger::errorf("Failed to publish session start: %s", sessionId.c_str());
    }

    return success;
}

bool MQTTManager::publishSessionEnd(const String& sessionId, const String& sessionType, const String& endReason,
                                   unsigned long duration, int totalMovements, int successfulMovements, int cycles) {
    if (!isConnected()) {
        REPORT_WARNING(ErrorCode::MQTT_CONNECTION_FAILED, "Cannot publish session end - MQTT not connected");
        return false;
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "session_end";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["session_type"] = sessionType;
    data["end_reason"] = endReason;
    data["total_duration"] = duration;
    data["movements_completed"] = totalMovements;
    data["successful_movements"] = successfulMovements;
    data["cycles_completed"] = cycles;

    bool success = publishJSON(TOPIC_SESSION_END, doc);
    if (success) {
        Logger::infof("Published session end: %s (Duration: %lu ms)", sessionId.c_str(), duration);
    } else {
        Logger::errorf("Failed to publish session end: %s", sessionId.c_str());
    }

    return success;
}

bool MQTTManager::publishSessionProgress(const String& sessionId, int completedCycles, int totalCycles,
                                        float progressPercent, int movementsCompleted) {
    if (!isConnected()) {
        return false; // Don't log warnings for progress updates
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "session_progress";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["completed_cycles"] = completedCycles;
    data["total_cycles"] = totalCycles;
    data["progress_percent"] = progressPercent;
    data["movements_completed"] = movementsCompleted;

    bool success = publishJSON(TOPIC_SESSION_PROGRESS, doc);
    if (success) {
        Logger::debugf("Published session progress: %s (%.1f%%)", sessionId.c_str(), progressPercent);
    }

    return success;
}

bool MQTTManager::publish(const char* topic, const String& payload, bool retain) {
    return publishWithRetry(topic, payload, retain);
}

bool MQTTManager::publishJSON(const char* topic, const JsonDocument& doc, bool retain) {
    String payload;
    serializeJson(doc, payload);
    return publishWithRetry(topic, payload, retain);
}

void MQTTManager::setConnectionCallback(void (*callback)(bool connected)) {
    connectionCallback = callback;
}

unsigned long MQTTManager::getConnectionTime() {
    if (isConnected() && connectedTime > 0) {
        return millis() - connectedTime;
    }
    return 0;
}

int MQTTManager::getReconnectionCount() {
    return reconnectionCount;
}

int MQTTManager::getPublishCount() {
    return publishCount;
}

int MQTTManager::getFailedPublishCount() {
    return failedPublishCount;
}

unsigned long MQTTManager::getLastPublishTime() {
    return lastPublishTime;
}

void MQTTManager::attemptConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning("Cannot connect MQTT - WiFi not connected");
        return;
    }

    if (currentStatus == MQTTStatus::CONNECTING) {
        return;  // Already attempting connection
    }

    Logger::info("Attempting MQTT connection...");

    currentStatus = MQTTStatus::CONNECTING;
    connectionStartTime = millis();
    lastReconnectAttempt = millis();
    connectionAttempts++;

    String clientId = String(DEVICE_ID) + "_" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
        Logger::info("MQTT connection successful!");
        currentStatus = MQTTStatus::CONNECTED;
        connectedTime = millis();
        connectionAttempts = 0;  // Reset on successful connection

        notifyConnectionChange(true);
    } else {
        Logger::errorf("MQTT connection failed, rc=%d", mqttClient.state());
        currentStatus = MQTTStatus::CONNECTION_FAILED;

        if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
            REPORT_ERROR(ErrorCode::MQTT_CONNECTION_FAILED,
                        "Max MQTT connection attempts reached");
            connectionAttempts = 0;  // Reset for next cycle
        }
    }
}

void MQTTManager::handleConnectionEvents() {
    static MQTTStatus lastStatus = MQTTStatus::DISCONNECTED;

    if (currentStatus != lastStatus) {
        logConnectionStatus();
        lastStatus = currentStatus;
    }
}

void MQTTManager::updateConnectionStatus() {
    if (currentStatus == MQTTStatus::CONNECTED && !mqttClient.connected()) {
        Logger::warning("MQTT connection lost");
        currentStatus = MQTTStatus::DISCONNECTED;
        notifyConnectionChange(false);
    }

    if (currentStatus == MQTTStatus::CONNECTING) {
        // Check for connection timeout
        if (millis() - connectionStartTime > CONNECTION_TIMEOUT) {
            Logger::error("MQTT connection timeout");
            currentStatus = MQTTStatus::CONNECTION_FAILED;
        }
    }
}

JsonDocument MQTTManager::createBaseDocument() {
    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = TimeManager::getCurrentTimestamp();
    doc["data"] = JsonObject();
    return doc;
}

bool MQTTManager::publishWithRetry(const char* topic, const String& payload, bool retain) {
    if (!isConnected()) {
        failedPublishCount++;
        return false;
    }

    for (int attempt = 0; attempt < PUBLISH_RETRY_COUNT; attempt++) {
        if (mqttClient.publish(topic, payload.c_str(), retain)) {
            publishCount++;
            lastPublishTime = millis();
            return true;
        }

        Logger::warningf("MQTT publish attempt %d failed", attempt + 1);
        delay(100);  // Brief delay between retries
    }

    failedPublishCount++;
    REPORT_WARNING(ErrorCode::MQTT_CONNECTION_FAILED, "MQTT publish failed after retries");
    return false;
}

void MQTTManager::logConnectionStatus() {
    switch (currentStatus) {
        case MQTTStatus::DISCONNECTED:
            Logger::info("MQTT Status: Disconnected");
            break;
        case MQTTStatus::CONNECTING:
            Logger::info("MQTT Status: Connecting...");
            break;
        case MQTTStatus::CONNECTED:
            Logger::info("MQTT Status: Connected");
            break;
        case MQTTStatus::CONNECTION_FAILED:
            Logger::warning("MQTT Status: Connection Failed");
            break;
        case MQTTStatus::RECONNECTING:
            Logger::info("MQTT Status: Reconnecting...");
            break;
    }
}

void MQTTManager::notifyConnectionChange(bool connected) {
    if (connectionCallback) {
        connectionCallback(connected);
    }

    if (connected) {
        reconnectionCount++;
    }
}

// Enhanced analytics publishing methods implementation
bool MQTTManager::publishMovementIndividual(int servoIndex, unsigned long startTime, unsigned long duration,
                                           bool successful, int startAngle, int targetAngle, int actualAngle,
                                           float smoothness, const String& movementType, const String& sessionId) {
    if (!isConnected()) {
        REPORT_WARNING(ErrorCode::MQTT_CONNECTION_FAILED, "Cannot publish - MQTT not connected");
        return false;
    }

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "movement_individual";

    JsonObject data = doc["data"].to<JsonObject>();
    data["servo_index"] = servoIndex;
    data["start_time"] = startTime;
    data["duration_ms"] = duration;
    data["successful"] = successful;
    data["start_angle"] = startAngle;
    data["target_angle"] = targetAngle;
    data["actual_angle"] = actualAngle;
    data["smoothness"] = smoothness;
    data["movement_type"] = movementType;
    if (sessionId.length() > 0) {
        data["session_id"] = sessionId;
    }

    bool success = publishJSON(TOPIC_MOVEMENT_INDIVIDUAL, doc);
    if (success) {
        Logger::debugf("Published individual movement: Servo %d, Duration %lu ms", servoIndex, duration);
    }

    return success;
}

bool MQTTManager::publishMovementQuality(const String& sessionId, float overallQuality,
                                        float averageSmoothness, float successRate) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "movement_quality";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["overall_quality"] = overallQuality;
    data["average_smoothness"] = averageSmoothness;
    data["success_rate"] = successRate;

    bool success = publishJSON(TOPIC_MOVEMENT_QUALITY, doc);
    if (success) {
        Logger::debugf("Published movement quality: Session %s, Quality %.2f", sessionId.c_str(), overallQuality);
    }

    return success;
}

bool MQTTManager::publishPerformanceTiming(unsigned long loopTime, unsigned long averageLoopTime,
                                          unsigned long maxLoopTime) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "performance_timing";

    JsonObject data = doc["data"].to<JsonObject>();
    data["current_loop_time"] = loopTime;
    data["average_loop_time"] = averageLoopTime;
    data["max_loop_time"] = maxLoopTime;

    bool success = publishJSON(TOPIC_PERFORMANCE_TIMING, doc);
    if (success) {
        Logger::debugf("Published performance timing: Current %lu ms, Avg %lu ms", loopTime, averageLoopTime);
    }

    return success;
}

bool MQTTManager::publishPerformanceMemory(size_t freeHeap, size_t minFreeHeap, float memoryUsagePercent) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "performance_memory";

    JsonObject data = doc["data"].to<JsonObject>();
    data["free_heap"] = freeHeap;
    data["min_free_heap"] = minFreeHeap;
    data["memory_usage_percent"] = memoryUsagePercent;

    bool success = publishJSON(TOPIC_PERFORMANCE_MEMORY, doc);
    if (success) {
        Logger::debugf("Published memory performance: Free %zu bytes, Usage %.1f%%", freeHeap, memoryUsagePercent);
    }

    return success;
}

bool MQTTManager::publishClinicalProgress(const String& sessionId, float progressScore,
                                         const String& progressIndicators) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "clinical_progress";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["progress_score"] = progressScore;
    data["progress_indicators"] = progressIndicators;

    bool success = publishJSON(TOPIC_CLINICAL_PROGRESS, doc);
    if (success) {
        Logger::debugf("Published clinical progress: Session %s, Score %.2f", sessionId.c_str(), progressScore);
    }

    return success;
}

bool MQTTManager::publishClinicalQuality(const String& sessionId, float sessionQuality,
                                        const String& qualityMetrics) {
    if (!isConnected()) return false;

    JsonDocument doc = createBaseDocument();
    doc["event_type"] = "clinical_quality";

    JsonObject data = doc["data"].to<JsonObject>();
    data["session_id"] = sessionId;
    data["session_quality"] = sessionQuality;
    data["quality_metrics"] = qualityMetrics;

    bool success = publishJSON(TOPIC_CLINICAL_QUALITY, doc);
    if (success) {
        Logger::debugf("Published clinical quality: Session %s, Quality %.2f", sessionId.c_str(), sessionQuality);
    }

    return success;
}

void MQTTManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down MQTT Manager...");

    stopTasks();
    disconnect();

    initialized = false;
    Logger::info("MQTT Manager shutdown complete");
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void MQTTManager::startTasks() {
    if (tasksRunning || publisherTaskHandle || subscriberTaskHandle) return;

    // Create MQTT Publisher Task
    BaseType_t publisherResult = xTaskCreatePinnedToCore(
        mqttPublisherTask,
        "MQTTPublisher",
        TASK_STACK_MQTT_PUBLISHER,
        this,
        PRIORITY_MQTT_PUBLISHER,
        &publisherTaskHandle,
        CORE_PROTOCOL  // Core 0 for protocol tasks
    );

    // Create MQTT Subscriber Task
    BaseType_t subscriberResult = xTaskCreatePinnedToCore(
        mqttSubscriberTask,
        "MQTTSubscriber",
        TASK_STACK_MQTT_SUBSCRIBER,
        this,
        PRIORITY_MQTT_SUBSCRIBER,
        &subscriberTaskHandle,
        CORE_PROTOCOL  // Core 0 for protocol tasks
    );

    if (publisherResult == pdPASS && subscriberResult == pdPASS) {
        tasksRunning = true;
        Logger::info("MQTT Publisher and Subscriber tasks started on Core 0");
    } else {
        Logger::error("Failed to create MQTT tasks");
        stopTasks();  // Clean up any partially created tasks
    }
}

void MQTTManager::stopTasks() {
    if (!tasksRunning) return;

    tasksRunning = false;

    if (publisherTaskHandle) {
        vTaskDelete(publisherTaskHandle);
        publisherTaskHandle = nullptr;
    }

    if (subscriberTaskHandle) {
        vTaskDelete(subscriberTaskHandle);
        subscriberTaskHandle = nullptr;
    }

    Logger::info("MQTT tasks stopped");
}

bool MQTTManager::areTasksRunning() {
    return tasksRunning && publisherTaskHandle != nullptr && subscriberTaskHandle != nullptr;
}

// Queue-based publishing for thread-safe operations
bool MQTTManager::queueMessage(const char* topic, const String& payload, bool retain, uint8_t priority) {
    // For now, fall back to direct publishing since FreeRTOS Manager is disabled
    // TODO: Implement proper queue-based publishing when FreeRTOS Manager is re-enabled
    return publishWithRetry(topic, payload, retain);
}

// =============================================================================
// FREERTOS TASK FUNCTIONS
// =============================================================================

void MQTTManager::mqttPublisherTask(void* parameter) {
    MQTTManager* manager = (MQTTManager*)parameter;
    Logger::info("MQTT Publisher task started");

    while (manager->tasksRunning) {
        // TODO: Process messages from publish queue when FreeRTOS Manager is re-enabled
        // For now, just maintain connection and handle basic operations

        if (manager->isConnected()) {
            // Publisher task is ready for queue-based publishing
            // Currently using direct publishing through existing methods
        }

        // Feed watchdog - now that FreeRTOS Manager is re-enabled
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());

        // Publisher runs at 100Hz for responsive publishing
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    Logger::info("MQTT Publisher task ended");
    vTaskDelete(nullptr);  // Delete self
}

void MQTTManager::mqttSubscriberTask(void* parameter) {
    MQTTManager* manager = (MQTTManager*)parameter;
    Logger::info("MQTT Subscriber task started");

    while (manager->tasksRunning) {
        // Handle MQTT client loop and connection management
        if (manager->mqttClient.connected()) {
            manager->mqttClient.loop();  // Process incoming messages
        }

        // Handle connection events and status updates
        manager->handleConnectionEvents();
        manager->updateConnectionStatus();

        // Handle reconnection logic
        if ((manager->currentStatus == MQTTStatus::DISCONNECTED ||
             manager->currentStatus == MQTTStatus::CONNECTION_FAILED) &&
            WiFi.status() == WL_CONNECTED) {

            unsigned long now = millis();
            if (now - manager->lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL) {
                manager->attemptConnection();
            }
        }

        // Feed watchdog - now that FreeRTOS Manager is re-enabled
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());

        // Subscriber runs at 20Hz for connection management
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    Logger::info("MQTT Subscriber task ended");
    vTaskDelete(nullptr);  // Delete self
}

// =============================================================================
// BIOMETRIC DATA PUBLISHING
// =============================================================================

bool MQTTManager::publishHeartRate(float heartRate, float spO2, const String& quality,
                                  bool fingerDetected, const String& sessionId) {
    if (!isConnected()) return false;

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["heart_rate"] = heartRate;
    doc["spo2"] = spO2;
    doc["signal_quality"] = quality;
    doc["finger_detected"] = fingerDetected;
    doc["session_id"] = sessionId;

    String payload;
    serializeJson(doc, payload);

    bool success = publish(TOPIC_SENSOR_HEART_RATE, payload);

    if (success) {
        Logger::infof("Published heart rate: %.1f BPM, SpO2: %.1f%%, Quality: %s",
                      heartRate, spO2, quality.c_str());
    } else {
        Logger::warningf("Failed to publish heart rate: %.1f BPM, SpO2: %.1f%%",
                        heartRate, spO2);
    }

    return success;
}

bool MQTTManager::publishPulseMetrics(const String& sessionId, float avgHeartRate, float minHeartRate,
                                     float maxHeartRate, float avgSpO2, float dataQuality) {
    if (!isConnected()) return false;

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = millis();
    doc["session_id"] = sessionId;
    doc["avg_heart_rate"] = avgHeartRate;
    doc["min_heart_rate"] = minHeartRate;
    doc["max_heart_rate"] = maxHeartRate;
    doc["avg_spo2"] = avgSpO2;
    doc["data_quality"] = dataQuality;

    String payload;
    serializeJson(doc, payload);

    bool success = publish("rehab_exo/pulse_metrics", payload);

    if (success) {
        Logger::debugf("Published pulse metrics for session: %s", sessionId.c_str());
    }

    return success;
}