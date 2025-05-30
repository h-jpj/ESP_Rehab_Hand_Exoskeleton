#include "SessionManager.h"
#include "../utils/ErrorHandler.h"

void SessionManager::initialize() {
    if (initialized) return;

    Logger::info("Initializing Session Manager...");

    // Initialize state
    currentState = SessionState::IDLE;
    autoStartEnabled = true;
    sessionTimeoutMs = DEFAULT_SESSION_TIMEOUT;
    minSessionIntervalMs = DEFAULT_MIN_INTERVAL;

    // Initialize timing
    lastSessionEndTime = 0;
    lastActivityTime = 0;
    sessionStartMillis = 0;

    // Initialize statistics
    sessionsToday = 0;
    totalSessionTime = 0;
    lastSessionDuration = 0;

    // Initialize callbacks
    sessionStartCallback = nullptr;
    sessionEndCallback = nullptr;
    sessionStateChangeCallback = nullptr;

    // Reset session data
    resetSessionData();

    initialized = true;
    Logger::info("Session Manager initialized successfully");
}

void SessionManager::update() {
    if (!initialized) return;

    // Check for session timeout
    if (currentState == SessionState::ACTIVE && shouldTimeoutSession()) {
        Logger::warning("Session timeout detected - ending session");
        endSession("timeout");
    }

    // Update activity tracking
    if (currentState == SessionState::ACTIVE) {
        updateSessionActivity();
    }
}

bool SessionManager::startSession(bool bleConnected) {
    if (!initialized) {
        Logger::error("SessionManager not initialized");
        return false;
    }

    if (currentState != SessionState::IDLE) {
        Logger::warning("Cannot start session - session already active");
        return false;
    }

    if (!canStartNewSession()) {
        Logger::warning("Cannot start session - minimum interval not met");
        return false;
    }

    // Initialize new session
    currentSession.sessionId = generateSessionId();
    currentSession.startTime = TimeManager::getCurrentTimestamp();
    currentSession.endTime = 0;

    // Store start time in millis for duration calculation
    sessionStartMillis = millis();
    currentSession.state = SessionState::ACTIVE;
    currentSession.type = SessionType::UNKNOWN;
    currentSession.totalMovements = 0;
    currentSession.successfulMovements = 0;
    currentSession.totalCycles = 0;
    currentSession.sequentialCommands = 0;
    currentSession.simultaneousCommands = 0;
    currentSession.testCommands = 0;
    currentSession.bleConnected = bleConnected;

    setState(SessionState::ACTIVE);
    lastActivityTime = millis();

    logSessionStart();

    // Notify via callback
    if (sessionStartCallback) {
        sessionStartCallback(currentSession.sessionId);
    }

    Logger::infof("Session started: %s", currentSession.sessionId.c_str());
    return true;
}

bool SessionManager::endSession(const String& reason) {
    if (!initialized) {
        Logger::error("SessionManager not initialized");
        return false;
    }

    if (currentState != SessionState::ACTIVE && currentState != SessionState::ENDING) {
        Logger::warning("Cannot end session - no active session");
        return false;
    }

    // Calculate session stats
    SessionStats stats;
    stats.duration = millis() - sessionStartMillis;
    stats.totalMovements = currentSession.totalMovements;
    stats.successfulMovements = currentSession.successfulMovements;
    stats.completedCycles = currentSession.totalCycles;
    stats.detectedType = detectSessionType();
    stats.endReason = reason;

    // Update session info
    currentSession.endTime = TimeManager::getCurrentTimestamp();
    currentSession.type = stats.detectedType;

    // Determine final state
    SessionState finalState = (reason == "user_requested") ? SessionState::COMPLETED : SessionState::INTERRUPTED;
    setState(finalState);

    // Update statistics
    lastSessionEndTime = millis();
    lastSessionDuration = stats.duration;
    totalSessionTime += stats.duration;
    sessionsToday++;

    logSessionEnd(stats);

    // Notify via callback
    if (sessionEndCallback) {
        sessionEndCallback(currentSession.sessionId, stats);
    }

    Logger::infof("Session ended: %s (Reason: %s, Duration: %lu ms)",
                 currentSession.sessionId.c_str(), reason.c_str(), stats.duration);

    // Reset to idle state
    setState(SessionState::IDLE);
    resetSessionData();

    return true;
}

bool SessionManager::isSessionActive() {
    return (currentState == SessionState::ACTIVE || currentState == SessionState::ENDING);
}

String SessionManager::getCurrentSessionId() {
    return isSessionActive() ? currentSession.sessionId : "";
}

SessionState SessionManager::getCurrentState() {
    return currentState;
}

SessionType SessionManager::getCurrentType() {
    return isSessionActive() ? detectSessionType() : SessionType::UNKNOWN;
}

SessionInfo SessionManager::getCurrentSessionInfo() {
    return currentSession;
}

SessionStats SessionManager::getSessionStats() {
    SessionStats stats;
    if (isSessionActive()) {
        stats.duration = millis() - sessionStartMillis;
        stats.totalMovements = currentSession.totalMovements;
        stats.successfulMovements = currentSession.successfulMovements;
        stats.completedCycles = currentSession.totalCycles;
        stats.detectedType = detectSessionType();
        stats.endReason = "ongoing";
    } else {
        // Return empty stats for inactive session
        stats.duration = 0;
        stats.totalMovements = 0;
        stats.successfulMovements = 0;
        stats.completedCycles = 0;
        stats.detectedType = SessionType::UNKNOWN;
        stats.endReason = "no_active_session";
    }
    return stats;
}

void SessionManager::recordMovementCommand(const String& command, bool successful) {
    if (!isSessionActive()) return;

    if (isValidMovementCommand(command)) {
        currentSession.totalMovements++;

        if (successful) {
            currentSession.successfulMovements++;
        }

        // Track command types for session type detection
        if (command == "1") {
            currentSession.sequentialCommands++;
        } else if (command == "2") {
            currentSession.simultaneousCommands++;
        } else if (command == "TEST") {
            currentSession.testCommands++;
        }

        // Update activity time
        lastActivityTime = millis();

        Logger::debugf("Movement recorded: %s (Success: %s, Total: %d)",
                      command.c_str(), successful ? "Yes" : "No", currentSession.totalMovements);
    }
}

void SessionManager::recordMovementComplete(int cycles) {
    if (!isSessionActive()) return;

    // Add cycles to total count
    currentSession.totalCycles += cycles;
    lastActivityTime = millis();

    Logger::debugf("Movement cycles completed: %d (Total: %d)", cycles, currentSession.totalCycles);
}

// Configuration methods
void SessionManager::setAutoStartEnabled(bool enabled) {
    autoStartEnabled = enabled;
    Logger::infof("Auto-start %s", enabled ? "enabled" : "disabled");
}

void SessionManager::setSessionTimeout(unsigned long timeoutMs) {
    sessionTimeoutMs = timeoutMs;
    Logger::infof("Session timeout set to %lu ms", timeoutMs);
}

void SessionManager::setMinSessionInterval(unsigned long intervalMs) {
    minSessionIntervalMs = intervalMs;
    Logger::infof("Minimum session interval set to %lu ms", intervalMs);
}

// Callback setters
void SessionManager::setSessionStartCallback(void (*callback)(const String& sessionId)) {
    sessionStartCallback = callback;
}

void SessionManager::setSessionEndCallback(void (*callback)(const String& sessionId, const SessionStats& stats)) {
    sessionEndCallback = callback;
}

void SessionManager::setSessionStateChangeCallback(void (*callback)(SessionState oldState, SessionState newState)) {
    sessionStateChangeCallback = callback;
}

// Statistics
unsigned long SessionManager::getLastSessionDuration() {
    return lastSessionDuration;
}

int SessionManager::getTotalSessionsToday() {
    return sessionsToday;
}

float SessionManager::getAverageSessionDuration() {
    if (sessionsToday == 0) return 0.0f;
    return (float)totalSessionTime / (float)sessionsToday;
}

// Private methods implementation

String SessionManager::generateSessionId() {
    // Generate session ID in format: SES_YYYYMMDD_XXX
    unsigned long timestamp = TimeManager::getCurrentTimestamp();

    // Simple session counter (resets daily)
    static int sessionCounter = 1;
    static unsigned long lastDay = 0;

    unsigned long currentDay = timestamp / 86400; // Days since epoch
    if (currentDay != lastDay) {
        sessionCounter = 1;
        lastDay = currentDay;
    }

    char sessionId[32];
    snprintf(sessionId, sizeof(sessionId), "SES_%08lX_%03d", (unsigned long)timestamp, sessionCounter++);

    return String(sessionId);
}

SessionType SessionManager::detectSessionType() {
    if (!isSessionActive()) return SessionType::UNKNOWN;

    int totalCommands = currentSession.sequentialCommands + currentSession.simultaneousCommands;

    // If only test commands
    if (currentSession.testCommands > 0 && totalCommands == 0) {
        return SessionType::TEST_ONLY;
    }

    // If no movement commands yet
    if (totalCommands == 0) {
        return SessionType::UNKNOWN;
    }

    // If both types of commands
    if (currentSession.sequentialCommands > 0 && currentSession.simultaneousCommands > 0) {
        return SessionType::MIXED;
    }

    // If only one type
    if (currentSession.sequentialCommands > 0) {
        return SessionType::SEQUENTIAL;
    }

    if (currentSession.simultaneousCommands > 0) {
        return SessionType::SIMULTANEOUS;
    }

    return SessionType::UNKNOWN;
}

void SessionManager::setState(SessionState newState) {
    if (newState != currentState) {
        SessionState oldState = currentState;
        currentState = newState;
        currentSession.state = newState;

        logStateChange(oldState, newState);

        if (sessionStateChangeCallback) {
            sessionStateChangeCallback(oldState, newState);
        }
    }
}

void SessionManager::resetSessionData() {
    currentSession.sessionId = "";
    currentSession.startTime = 0;
    currentSession.endTime = 0;
    sessionStartMillis = 0;
    currentSession.state = SessionState::IDLE;
    currentSession.type = SessionType::UNKNOWN;
    currentSession.totalMovements = 0;
    currentSession.successfulMovements = 0;
    currentSession.totalCycles = 0;
    currentSession.sequentialCommands = 0;
    currentSession.simultaneousCommands = 0;
    currentSession.testCommands = 0;
    currentSession.bleConnected = false;
}

bool SessionManager::shouldTimeoutSession() {
    if (!isSessionActive()) return false;

    unsigned long timeSinceActivity = millis() - lastActivityTime;
    return (timeSinceActivity > sessionTimeoutMs);
}

bool SessionManager::canStartNewSession() {
    if (lastSessionEndTime == 0) return true; // First session

    unsigned long timeSinceLastSession = millis() - lastSessionEndTime;
    return (timeSinceLastSession >= minSessionIntervalMs);
}

void SessionManager::updateSessionActivity() {
    // This method can be used for periodic activity checks
    // Currently just maintains the activity timestamp
}

bool SessionManager::isValidMovementCommand(const String& command) {
    return (command == "0" || command == "1" || command == "2" || command == "TEST");
}

void SessionManager::logSessionStart() {
    Logger::infof("=== SESSION STARTED ===");
    Logger::infof("Session ID: %s", currentSession.sessionId.c_str());
    Logger::infof("Start Time: %lu", currentSession.startTime);
    Logger::infof("BLE Connected: %s", currentSession.bleConnected ? "Yes" : "No");
}

void SessionManager::logSessionEnd(const SessionStats& stats) {
    Logger::infof("=== SESSION ENDED ===");
    Logger::infof("Session ID: %s", currentSession.sessionId.c_str());
    Logger::infof("Duration: %lu ms", stats.duration);
    Logger::infof("Total Movements: %d", stats.totalMovements);
    Logger::infof("Successful Movements: %d", stats.successfulMovements);
    Logger::infof("Total Cycles: %d", stats.completedCycles);
    Logger::infof("Session Type: %d", (int)stats.detectedType);
    Logger::infof("End Reason: %s", stats.endReason.c_str());
}

void SessionManager::logStateChange(SessionState oldState, SessionState newState) {
    Logger::infof("Session state changed: %d -> %d", (int)oldState, (int)newState);
}
