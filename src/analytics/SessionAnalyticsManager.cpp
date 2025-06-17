#include "SessionAnalyticsManager.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"

void SessionAnalyticsManager::initialize() {
    if (initialized) return;
    
    Logger::info("Initializing Session Analytics Manager...");
    
    taskRunning = false;
    taskHandle = nullptr;
    analyticsQueue = nullptr;
    currentSessionId = "";
    newAnalyticsAvailable = false;
    processedEvents = 0;
    queuedEvents = 0;
    lastProcessingTime = 0;
    processingTimeTotal = 0;
    processingCount = 0;
    activeSessionCount = 0;
    movementHistoryIndex = 0;
    analyticsCallback = nullptr;
    
    // Initialize session data
    for (int i = 0; i < 5; i++) {
        activeSessions[i] = {};
    }
    
    // Initialize movement history
    for (int i = 0; i < 50; i++) {
        recentMovements[i] = {};
    }
    
    // Create analytics queue
    analyticsQueue = xQueueCreate(ANALYTICS_QUEUE_SIZE, sizeof(AnalyticsEvent));
    if (analyticsQueue == nullptr) {
        Logger::error("Failed to create analytics queue");
        return;
    }
    
    initialized = true;
    
    // Start the session analytics task
    startTask();
    
    Logger::info("Session Analytics Manager initialized with FreeRTOS task");
}

void SessionAnalyticsManager::shutdown() {
    if (!initialized) return;
    
    Logger::info("Shutting down Session Analytics Manager...");
    
    stopTask();
    
    if (analyticsQueue) {
        vQueueDelete(analyticsQueue);
        analyticsQueue = nullptr;
    }
    
    initialized = false;
    Logger::info("Session Analytics Manager shutdown complete");
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void SessionAnalyticsManager::startTask() {
    if (taskRunning || taskHandle) return;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        sessionAnalyticsTask,
        "SessionAnalytics",
        TASK_STACK_SESSION_ANALYTICS,
        this,
        PRIORITY_SESSION_ANALYTICS,
        &taskHandle,
        CORE_APPLICATION  // Core 1 for application tasks
    );
    
    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("Session Analytics task started on Core 1");
    } else {
        Logger::error("Failed to create Session Analytics task");
    }
}

void SessionAnalyticsManager::stopTask() {
    if (!taskRunning || !taskHandle) return;
    
    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
    
    Logger::info("Session Analytics task stopped");
}

bool SessionAnalyticsManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void SessionAnalyticsManager::sessionAnalyticsTask(void* parameter) {
    SessionAnalyticsManager* manager = (SessionAnalyticsManager*)parameter;
    Logger::info("Session Analytics task started");
    
    while (manager->taskRunning) {
        // Process analytics queue
        manager->processAnalyticsQueue();
        
        // Update real-time metrics
        if (manager->currentSessionId.length() > 0) {
            manager->generateSessionQuality(manager->currentSessionId);
            manager->generateClinicalProgress(manager->currentSessionId);
        }
        
        // Publish analytics if new data available
        if (manager->hasNewAnalytics()) {
            manager->publishAnalytics(manager->currentSessionId);
            manager->clearNewAnalytics();
        }
        
        // Feed watchdog (when FreeRTOS Manager is re-enabled)
        // FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());
        
        // Analytics processing runs every 100ms for responsiveness
        vTaskDelay(pdMS_TO_TICKS(ANALYTICS_PROCESSING_INTERVAL));
    }
    
    Logger::info("Session Analytics task ended");
    vTaskDelete(nullptr);  // Delete self
}

// =============================================================================
// ANALYTICS PROCESSING
// =============================================================================

void SessionAnalyticsManager::processMovementData(const MovementAnalytics& movement) {
    AnalyticsEvent event;
    event.type = AnalyticsEventType::MOVEMENT_INDIVIDUAL;
    event.sessionId = movement.sessionId;
    event.timestamp = millis();
    event.movement = new MovementAnalytics(movement);

    queueAnalyticsEvent(event);
}

void SessionAnalyticsManager::processSessionStart(const String& sessionId) {
    AnalyticsEvent event;
    event.type = AnalyticsEventType::SESSION_START;
    event.sessionId = sessionId;
    event.timestamp = millis();
    // No additional data needed for session start

    queueAnalyticsEvent(event);
}

void SessionAnalyticsManager::processSessionEnd(const String& sessionId, unsigned long duration) {
    AnalyticsEvent event;
    event.type = AnalyticsEventType::SESSION_END;
    event.sessionId = sessionId;
    event.timestamp = millis();
    // Duration could be stored in a simple field if needed

    queueAnalyticsEvent(event);
}

bool SessionAnalyticsManager::queueAnalyticsEvent(const AnalyticsEvent& event) {
    if (!analyticsQueue) return false;
    
    BaseType_t result = xQueueSend(analyticsQueue, &event, pdMS_TO_TICKS(10));
    if (result == pdPASS) {
        queuedEvents++;
        return true;
    } else {
        Logger::warning("Analytics queue full, dropping event");
        return false;
    }
}

void SessionAnalyticsManager::processAnalyticsQueue() {
    AnalyticsEvent event;
    
    // Process all queued events
    while (xQueueReceive(analyticsQueue, &event, 0) == pdPASS) {
        unsigned long processingStart = millis();
        
        processEvent(event);
        processedEvents++;
        
        // Track processing performance
        unsigned long processingTime = millis() - processingStart;
        processingTimeTotal += processingTime;
        processingCount++;
    }
}

void SessionAnalyticsManager::processEvent(const AnalyticsEvent& event) {
    switch (event.type) {
        case AnalyticsEventType::SESSION_START:
            handleSessionStart(event);
            break;
            
        case AnalyticsEventType::SESSION_END:
            handleSessionEnd(event);
            break;
            
        case AnalyticsEventType::MOVEMENT_INDIVIDUAL:
            handleMovementData(event);
            break;
            
        case AnalyticsEventType::MOVEMENT_QUALITY:
            handleQualityUpdate(event);
            break;
            
        case AnalyticsEventType::CLINICAL_PROGRESS:
            handleProgressUpdate(event);
            break;
            
        default:
            Logger::warningf("Unknown analytics event type: %d", (int)event.type);
            break;
    }
    
    // Notify via callback if set
    if (analyticsCallback) {
        analyticsCallback(event);
    }
}

// =============================================================================
// SESSION ANALYTICS
// =============================================================================

float SessionAnalyticsManager::calculateMovementQuality(const MovementAnalytics& movement) {
    float quality = 0.0f;
    
    // Base quality on success
    if (movement.successful) {
        quality += 0.5f; // 50% for success
    }
    
    // Add smoothness component (30%)
    quality += movement.smoothness * 0.3f;
    
    // Add timing component (20%)
    float timingScore = 1.0f;
    if (movement.duration > 2000) { // Slower than 2 seconds
        timingScore = 0.5f;
    } else if (movement.duration < 500) { // Faster than 0.5 seconds
        timingScore = 0.7f;
    }
    quality += timingScore * 0.2f;
    
    return constrain(quality, 0.0f, 1.0f);
}

SessionQualityMetrics SessionAnalyticsManager::getSessionQuality(const String& sessionId) {
    return currentSessionMetrics;
}

ClinicalProgressData SessionAnalyticsManager::getClinicalProgress(const String& sessionId) {
    return currentProgressData;
}

void SessionAnalyticsManager::generateSessionQuality(const String& sessionId) {
    SessionData* session = findSession(sessionId);
    if (!session || !session->active) return;
    
    currentSessionMetrics.sessionId = sessionId;
    currentSessionMetrics.overallQuality = calculateOverallQuality(sessionId);
    currentSessionMetrics.averageSmoothness = calculateAverageSmoothness(sessionId);
    currentSessionMetrics.successRate = calculateSuccessRate(sessionId);
    currentSessionMetrics.totalMovements = session->totalMovements;
    currentSessionMetrics.successfulMovements = session->successfulMovements;
    currentSessionMetrics.totalDuration = session->totalDuration;
    
    if (session->totalMovements > 0) {
        currentSessionMetrics.averageMovementTime = session->totalDuration / session->totalMovements;
    } else {
        currentSessionMetrics.averageMovementTime = 0;
    }
    
    newAnalyticsAvailable = true;
}

void SessionAnalyticsManager::generateClinicalProgress(const String& sessionId) {
    currentProgressData.sessionId = sessionId;
    currentProgressData.progressScore = calculateProgressScore(sessionId);
    currentProgressData.progressIndicators = generateProgressIndicators(sessionId);
    currentProgressData.qualityTrend = analyzeQualityTrend(sessionId);
    
    // Calculate improvement (simplified)
    currentProgressData.improvementPercent = currentProgressData.progressScore * 100.0f;
    
    newAnalyticsAvailable = true;
}

uint32_t SessionAnalyticsManager::getProcessedEvents() {
    return processedEvents;
}

uint32_t SessionAnalyticsManager::getQueuedEvents() {
    return queuedEvents;
}

float SessionAnalyticsManager::getProcessingRate() {
    if (processingCount == 0) return 0.0f;
    return (float)processingTimeTotal / processingCount;
}

bool SessionAnalyticsManager::hasNewAnalytics() {
    return newAnalyticsAvailable;
}

void SessionAnalyticsManager::clearNewAnalytics() {
    newAnalyticsAvailable = false;
}

void SessionAnalyticsManager::setAnalyticsCallback(void (*callback)(const AnalyticsEvent& event)) {
    analyticsCallback = callback;
}

// =============================================================================
// EVENT HANDLERS
// =============================================================================

void SessionAnalyticsManager::handleSessionStart(const AnalyticsEvent& event) {
    Logger::infof("Analytics: Session started - %s", event.sessionId.c_str());

    SessionData* session = createSession(event.sessionId);
    if (session) {
        session->startTime = event.timestamp;
        session->active = true;
        currentSessionId = event.sessionId;
    }
}

void SessionAnalyticsManager::handleSessionEnd(const AnalyticsEvent& event) {
    Logger::infof("Analytics: Session ended - %s", event.sessionId.c_str());

    SessionData* session = findSession(event.sessionId);
    if (session) {
        session->endTime = event.timestamp;
        session->active = false;

        // Generate final analytics
        generateSessionQuality(event.sessionId);
        generateClinicalProgress(event.sessionId);

        // Remove from active sessions
        removeSession(event.sessionId);
    }

    if (currentSessionId == event.sessionId) {
        currentSessionId = "";
    }
}

void SessionAnalyticsManager::handleMovementData(const AnalyticsEvent& event) {
    if (!event.movement) return;

    const MovementAnalytics& movement = *event.movement;

    // Update session metrics
    updateSessionMetrics(movement.sessionId, movement);

    // Add to movement history
    addMovementToHistory(movement);

    // Calculate and update real-time quality
    updateRealTimeMetrics(movement);

    Logger::debugf("Analytics: Movement processed - Servo %d, Quality %.2f",
                  movement.servoIndex, calculateMovementQuality(movement));
}

void SessionAnalyticsManager::handleQualityUpdate(const AnalyticsEvent& event) {
    // Handle quality update events
    Logger::debug("Analytics: Quality update processed");
}

void SessionAnalyticsManager::handleProgressUpdate(const AnalyticsEvent& event) {
    // Handle progress update events
    Logger::debug("Analytics: Progress update processed");
}

// =============================================================================
// SESSION MANAGEMENT
// =============================================================================

SessionAnalyticsManager::SessionData* SessionAnalyticsManager::findSession(const String& sessionId) {
    for (int i = 0; i < 5; i++) {
        if (activeSessions[i].sessionId == sessionId && activeSessions[i].active) {
            return &activeSessions[i];
        }
    }
    return nullptr;
}

SessionAnalyticsManager::SessionData* SessionAnalyticsManager::createSession(const String& sessionId) {
    // Find empty slot
    for (int i = 0; i < 5; i++) {
        if (!activeSessions[i].active) {
            activeSessions[i] = {};
            activeSessions[i].sessionId = sessionId;
            activeSessions[i].active = true;
            activeSessionCount++;
            return &activeSessions[i];
        }
    }

    Logger::warning("No available session slots");
    return nullptr;
}

void SessionAnalyticsManager::removeSession(const String& sessionId) {
    for (int i = 0; i < 5; i++) {
        if (activeSessions[i].sessionId == sessionId) {
            activeSessions[i] = {};
            activeSessionCount--;
            break;
        }
    }
}

void SessionAnalyticsManager::updateSessionMetrics(const String& sessionId, const MovementAnalytics& movement) {
    SessionData* session = findSession(sessionId);
    if (!session) return;

    session->totalMovements++;
    if (movement.successful) {
        session->successfulMovements++;
    }
    session->totalSmoothness += movement.smoothness;
    session->totalDuration += movement.duration;
}

void SessionAnalyticsManager::addMovementToHistory(const MovementAnalytics& movement) {
    MovementHistory& history = recentMovements[movementHistoryIndex];
    history.timestamp = millis();
    history.quality = calculateMovementQuality(movement);
    history.smoothness = movement.smoothness;
    history.successful = movement.successful;
    history.sessionId = movement.sessionId;

    movementHistoryIndex = (movementHistoryIndex + 1) % 50;
}

void SessionAnalyticsManager::updateRealTimeMetrics(const MovementAnalytics& movement) {
    // Update real-time session metrics
    generateSessionQuality(movement.sessionId);
    newAnalyticsAvailable = true;
}

// =============================================================================
// ANALYTICS CALCULATIONS
// =============================================================================

float SessionAnalyticsManager::calculateOverallQuality(const String& sessionId) {
    SessionData* session = findSession(sessionId);
    if (!session || session->totalMovements == 0) return 0.0f;

    float successRate = (float)session->successfulMovements / session->totalMovements;
    float averageSmoothness = session->totalSmoothness / session->totalMovements;

    // Combine success rate (60%) and smoothness (40%)
    return (successRate * 0.6f) + (averageSmoothness * 0.4f);
}

float SessionAnalyticsManager::calculateAverageSmoothness(const String& sessionId) {
    SessionData* session = findSession(sessionId);
    if (!session || session->totalMovements == 0) return 0.0f;

    return session->totalSmoothness / session->totalMovements;
}

float SessionAnalyticsManager::calculateSuccessRate(const String& sessionId) {
    SessionData* session = findSession(sessionId);
    if (!session || session->totalMovements == 0) return 0.0f;

    return (float)session->successfulMovements / session->totalMovements;
}

float SessionAnalyticsManager::calculateProgressScore(const String& sessionId) {
    // Simplified progress calculation based on current session quality
    return calculateOverallQuality(sessionId);
}

String SessionAnalyticsManager::generateProgressIndicators(const String& sessionId) {
    float quality = calculateOverallQuality(sessionId);

    if (quality >= QUALITY_EXCELLENT_THRESHOLD) {
        return "Excellent progress, maintaining high quality";
    } else if (quality >= QUALITY_GOOD_THRESHOLD) {
        return "Good progress, steady improvement";
    } else {
        return "Needs improvement, focus on consistency";
    }
}

String SessionAnalyticsManager::analyzeQualityTrend(const String& sessionId) {
    // Simplified trend analysis
    float recentTrend = getRecentQualityTrend(sessionId);

    if (recentTrend > 0.1f) {
        return "Improving";
    } else if (recentTrend < -0.1f) {
        return "Declining";
    } else {
        return "Stable";
    }
}

float SessionAnalyticsManager::getRecentQualityTrend(const String& sessionId) {
    // Calculate trend from recent movements
    float totalQuality = 0.0f;
    int count = 0;

    for (int i = 0; i < 50; i++) {
        if (recentMovements[i].sessionId == sessionId) {
            totalQuality += recentMovements[i].quality;
            count++;
        }
    }

    return count > 0 ? totalQuality / count : 0.0f;
}

void SessionAnalyticsManager::publishAnalytics(const String& sessionId) {
    // Publish session quality metrics
    publishSessionQuality(currentSessionMetrics);

    // Publish clinical progress data
    publishClinicalProgress(currentProgressData);

    Logger::debugf("Published analytics for session: %s", sessionId.c_str());
    Logger::debugf("Quality: %.2f, Success Rate: %.2f, Progress: %.2f",
                  currentSessionMetrics.overallQuality,
                  currentSessionMetrics.successRate,
                  currentProgressData.progressScore);
}

void SessionAnalyticsManager::publishSessionQuality(const SessionQualityMetrics& quality) {
    // This would integrate with MQTT Manager to publish session quality
    // For now, just log the quality metrics
    Logger::debugf("Session Quality - Overall: %.2f, Smoothness: %.2f, Success Rate: %.2f",
                  quality.overallQuality, quality.averageSmoothness, quality.successRate);
}

void SessionAnalyticsManager::publishClinicalProgress(const ClinicalProgressData& progress) {
    // This would integrate with MQTT Manager to publish clinical progress
    // For now, just log the progress data
    Logger::debugf("Clinical Progress - Score: %.2f, Trend: %s, Indicators: %s",
                  progress.progressScore, progress.qualityTrend.c_str(), progress.progressIndicators.c_str());
}

void SessionAnalyticsManager::publishMovementQuality(const MovementAnalytics& movement) {
    // This would integrate with MQTT Manager to publish movement quality
    // For now, just log the movement quality
    float quality = calculateMovementQuality(movement);
    Logger::debugf("Movement Quality - Servo %d: %.2f (Smoothness: %.2f, Success: %s)",
                  movement.servoIndex, quality, movement.smoothness,
                  movement.successful ? "Yes" : "No");
}
