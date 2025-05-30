#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <Arduino.h>
#include "../utils/Logger.h"
#include "../utils/TimeManager.h"

enum class SessionState {
    IDLE,           // No active session
    ACTIVE,         // Session running, accepting commands
    ENDING,         // End requested, finishing current movement
    COMPLETED,      // Session ended successfully by user
    INTERRUPTED     // Session ended due to disconnection/error
};

enum class SessionType {
    UNKNOWN,        // Type not yet determined
    SEQUENTIAL,     // Only sequential movements (command "1")
    SIMULTANEOUS,   // Only simultaneous movements (command "2")
    MIXED,          // Both sequential and simultaneous
    TEST_ONLY       // Only test commands
};

struct SessionStats {
    unsigned long duration;         // Session duration in milliseconds
    int totalMovements;            // Total movement commands received
    int successfulMovements;       // Successfully executed movements
    int completedCycles;           // Completed movement cycles
    SessionType detectedType;      // Detected session type
    String endReason;              // Reason for session end
};

struct SessionInfo {
    String sessionId;              // Unique session identifier
    unsigned long startTime;       // Session start timestamp (Unix)
    unsigned long endTime;         // Session end timestamp (Unix, 0 if ongoing)
    SessionState state;            // Current session state
    SessionType type;              // Session type (detected dynamically)
    int totalMovements;            // Total movement commands
    int successfulMovements;       // Successfully executed movements
    int totalCycles;               // Total completed cycles
    int sequentialCommands;        // Count of "1" commands
    int simultaneousCommands;      // Count of "2" commands
    int testCommands;              // Count of "TEST" commands
    bool bleConnected;             // BLE connection status at start
};

class SessionManager {
public:
    // Initialization and lifecycle
    void initialize();
    void update();

    // Session lifecycle
    bool startSession(bool bleConnected = true);
    bool endSession(const String& reason = "user_requested");
    bool isSessionActive();

    // Session information
    String getCurrentSessionId();
    SessionState getCurrentState();
    SessionType getCurrentType();
    SessionInfo getCurrentSessionInfo();
    SessionStats getSessionStats();

    // Movement tracking
    void recordMovementCommand(const String& command, bool successful);
    void recordMovementComplete(int cycles);

    // Configuration
    void setAutoStartEnabled(bool enabled);
    void setSessionTimeout(unsigned long timeoutMs);
    void setMinSessionInterval(unsigned long intervalMs);

    // Callbacks
    void setSessionStartCallback(void (*callback)(const String& sessionId));
    void setSessionEndCallback(void (*callback)(const String& sessionId, const SessionStats& stats));
    void setSessionStateChangeCallback(void (*callback)(SessionState oldState, SessionState newState));

    // Statistics
    unsigned long getLastSessionDuration();
    int getTotalSessionsToday();
    float getAverageSessionDuration();

private:
    bool initialized;
    SessionInfo currentSession;
    SessionState currentState;

    // Configuration
    bool autoStartEnabled;
    unsigned long sessionTimeoutMs;
    unsigned long minSessionIntervalMs;

    // Timing
    unsigned long lastSessionEndTime;
    unsigned long lastActivityTime;
    unsigned long sessionStartMillis;  // Session start time in millis() for duration calculation

    // Statistics
    int sessionsToday;
    unsigned long totalSessionTime;
    unsigned long lastSessionDuration;

    // Callbacks
    void (*sessionStartCallback)(const String& sessionId);
    void (*sessionEndCallback)(const String& sessionId, const SessionStats& stats);
    void (*sessionStateChangeCallback)(SessionState oldState, SessionState newState);

    // Internal methods
    String generateSessionId();
    SessionType detectSessionType();
    void setState(SessionState newState);
    void resetSessionData();
    bool shouldTimeoutSession();
    bool canStartNewSession();
    void updateSessionActivity();

    // Validation
    bool isValidMovementCommand(const String& command);

    // Logging
    void logSessionStart();
    void logSessionEnd(const SessionStats& stats);
    void logStateChange(SessionState oldState, SessionState newState);

    // Configuration constants
    static const unsigned long DEFAULT_SESSION_TIMEOUT = 300000;    // 5 minutes
    static const unsigned long DEFAULT_MIN_INTERVAL = 30000;        // 30 seconds
    static const unsigned long ACTIVITY_TIMEOUT = 10000;            // 10 seconds for activity tracking
};

#endif
