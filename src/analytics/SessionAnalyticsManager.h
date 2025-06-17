#ifndef SESSION_ANALYTICS_MANAGER_H
#define SESSION_ANALYTICS_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "../config/Config.h"

enum class AnalyticsEventType {
    SESSION_START,
    SESSION_END,
    MOVEMENT_COMMAND,
    MOVEMENT_INDIVIDUAL,
    MOVEMENT_QUALITY,
    CLINICAL_PROGRESS,
    PERFORMANCE_UPDATE
};

struct MovementAnalytics {
    int servoIndex;
    unsigned long startTime;
    unsigned long duration;
    bool successful;
    int startAngle;
    int targetAngle;
    int actualAngle;
    float smoothness;
    String movementType;
    String sessionId;
};

struct SessionQualityMetrics {
    String sessionId;
    float overallQuality;
    float averageSmoothness;
    float successRate;
    int totalMovements;
    int successfulMovements;
    unsigned long totalDuration;
    unsigned long averageMovementTime;
};

struct ClinicalProgressData {
    String sessionId;
    float progressScore;
    String progressIndicators;
    float improvementPercent;
    int consecutiveSuccessfulSessions;
    unsigned long sessionDuration;
    String qualityTrend;
};

struct AnalyticsEvent {
    AnalyticsEventType type;
    String sessionId;
    unsigned long timestamp;

    // Use separate optional fields instead of union
    MovementAnalytics* movement;
    SessionQualityMetrics* quality;
    ClinicalProgressData* progress;

    // Constructor
    AnalyticsEvent() : movement(nullptr), quality(nullptr), progress(nullptr) {}

    // Destructor
    ~AnalyticsEvent() {
        delete movement;
        delete quality;
        delete progress;
    }
};

class SessionAnalyticsManager {
public:
    // Initialization and lifecycle
    void initialize();
    void shutdown();
    
    // FreeRTOS task management
    void startTask();
    void stopTask();
    bool isTaskRunning();
    
    // Analytics processing
    void processMovementData(const MovementAnalytics& movement);
    void processSessionStart(const String& sessionId);
    void processSessionEnd(const String& sessionId, unsigned long duration);
    void generateSessionQuality(const String& sessionId);
    void generateClinicalProgress(const String& sessionId);
    
    // Queue-based event processing
    bool queueAnalyticsEvent(const AnalyticsEvent& event);
    void processAnalyticsQueue();
    
    // Session analytics
    SessionQualityMetrics getSessionQuality(const String& sessionId);
    ClinicalProgressData getClinicalProgress(const String& sessionId);
    float calculateMovementQuality(const MovementAnalytics& movement);
    float calculateSessionProgress(const String& sessionId);
    
    // Real-time analytics
    void updateRealTimeMetrics(const MovementAnalytics& movement);
    void publishAnalytics(const String& sessionId);
    bool hasNewAnalytics();
    void clearNewAnalytics();
    
    // Statistics
    uint32_t getProcessedEvents();
    uint32_t getQueuedEvents();
    float getProcessingRate();
    
    // Callbacks
    void setAnalyticsCallback(void (*callback)(const AnalyticsEvent& event));

private:
    bool initialized;
    bool taskRunning;
    TaskHandle_t taskHandle;
    QueueHandle_t analyticsQueue;
    
    // Analytics state
    String currentSessionId;
    SessionQualityMetrics currentSessionMetrics;
    ClinicalProgressData currentProgressData;
    bool newAnalyticsAvailable;
    
    // Performance tracking
    uint32_t processedEvents;
    uint32_t queuedEvents;
    unsigned long lastProcessingTime;
    uint32_t processingTimeTotal;
    uint32_t processingCount;
    
    // Session tracking
    struct SessionData {
        String sessionId;
        unsigned long startTime;
        unsigned long endTime;
        int totalMovements;
        int successfulMovements;
        float totalSmoothness;
        unsigned long totalDuration;
        bool active;
    };
    
    SessionData activeSessions[5]; // Support up to 5 concurrent sessions
    int activeSessionCount;
    
    // Movement history for trend analysis
    struct MovementHistory {
        unsigned long timestamp;
        float quality;
        float smoothness;
        bool successful;
        String sessionId;
    };
    
    MovementHistory recentMovements[50]; // Keep last 50 movements
    int movementHistoryIndex;
    
    // Task function
    static void sessionAnalyticsTask(void* parameter);
    
    // Callback
    void (*analyticsCallback)(const AnalyticsEvent& event);
    
    // Internal processing methods
    void processEvent(const AnalyticsEvent& event);
    void handleSessionStart(const AnalyticsEvent& event);
    void handleSessionEnd(const AnalyticsEvent& event);
    void handleMovementData(const AnalyticsEvent& event);
    void handleQualityUpdate(const AnalyticsEvent& event);
    void handleProgressUpdate(const AnalyticsEvent& event);
    
    // Analytics calculations
    float calculateOverallQuality(const String& sessionId);
    float calculateAverageSmoothness(const String& sessionId);
    float calculateSuccessRate(const String& sessionId);
    float calculateProgressScore(const String& sessionId);
    String generateProgressIndicators(const String& sessionId);
    String analyzeQualityTrend(const String& sessionId);
    
    // Session management
    SessionData* findSession(const String& sessionId);
    SessionData* createSession(const String& sessionId);
    void removeSession(const String& sessionId);
    void updateSessionMetrics(const String& sessionId, const MovementAnalytics& movement);
    
    // Movement history management
    void addMovementToHistory(const MovementAnalytics& movement);
    float getRecentQualityTrend(const String& sessionId);
    int getRecentSuccessCount(const String& sessionId);
    
    // Publishing helpers
    void publishSessionQuality(const SessionQualityMetrics& quality);
    void publishClinicalProgress(const ClinicalProgressData& progress);
    void publishMovementQuality(const MovementAnalytics& movement);
    
    // Configuration
    static const uint32_t ANALYTICS_QUEUE_SIZE = 20;
    static const uint32_t ANALYTICS_PROCESSING_INTERVAL = 100; // 100ms
    static constexpr float QUALITY_EXCELLENT_THRESHOLD = 0.9f;
    static constexpr float QUALITY_GOOD_THRESHOLD = 0.7f;
    static constexpr float SMOOTHNESS_EXCELLENT_THRESHOLD = 0.85f;
    static constexpr float SUCCESS_RATE_EXCELLENT_THRESHOLD = 0.95f;
};

#endif
