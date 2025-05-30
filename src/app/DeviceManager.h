#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>

// Include all component managers
#include "../network/WiFiManager.h"
#include "../network/MQTTManager.h"
#include "../bluetooth/BLEManager.h"
#include "../hardware/ServoController.h"
#include "../hardware/SystemMonitor.h"
#include "../utils/TimeManager.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "CommandProcessor.h"
#include "SessionManager.h"

enum class DeviceState {
    INITIALIZING,
    READY,
    RUNNING,
    ERROR,
    MAINTENANCE
};

class DeviceManager {
public:
    // Lifecycle management
    void initialize();
    void update();
    void shutdown();

    // State management
    DeviceState getState();
    bool isReady();
    bool isHealthy();

    // Component access
    WiFiManager& getWiFiManager();
    MQTTManager& getMQTTManager();
    BLEManager& getBLEManager();
    ServoController& getServoController();
    SystemMonitor& getSystemMonitor();
    CommandProcessor& getCommandProcessor();
    SessionManager& getSessionManager();

    // Command handling
    bool handleCommand(const String& command, CommandSource source);
    void processQueuedCommands();

    // Status reporting
    void publishSystemStatus();
    void logSystemSummary();

    // Configuration
    void setStatusReportInterval(unsigned long interval);
    void setLogLevel(LogLevel level);

    // Callbacks
    void setStateChangeCallback(void (*callback)(DeviceState oldState, DeviceState newState));

private:
    // Component instances
    WiFiManager wifiManager;
    MQTTManager mqttManager;
    BLEManager bleManager;
    ServoController servoController;
    SystemMonitor systemMonitor;
    CommandProcessor commandProcessor;
    SessionManager sessionManager;

    // State management
    DeviceState currentState;
    bool initialized;
    unsigned long lastStatusReport;
    unsigned long statusReportInterval;
    unsigned long lastUpdate;

    // Performance tracking
    unsigned long loopStartTime;
    unsigned long totalLoopTime;
    unsigned long loopCount;

    // Callbacks
    void (*stateChangeCallback)(DeviceState oldState, DeviceState newState);

    // Initialization phases
    bool initializeFoundation();
    bool initializeCommunication();
    bool initializeHardware();
    bool initializeApplication();

    // Update phases
    void updateCommunication();
    void updateHardware();
    void updateApplication();
    void updateMonitoring();

    // Command handling
    bool executeMovementCommand(const Command& command);
    bool executeSystemCommand(const Command& command);
    void onCommandComplete(const Command& command, bool success);
    void onCommandError(const Command& command, const String& error);

    // Connection callbacks
    static void onWiFiConnectionChange(bool connected);
    static void onMQTTConnectionChange(bool connected);
    static void onBLEConnectionChange(bool connected);
    static void onBLECommandReceived(const String& command);
    static void onServoMovementComplete(ServoState state, int cycles);
    static void onSystemAlert(SystemHealth health, const String& message);

    // Session callbacks
    static void onSessionStart(const String& sessionId);
    static void onSessionEnd(const String& sessionId, const SessionStats& stats);
    static void onSessionStateChange(SessionState oldState, SessionState newState);

    // State management
    void setState(DeviceState newState);
    void checkSystemHealth();
    void handleSystemError();

    // Status publishing
    void publishMovementStatus(const Command& command, unsigned long responseTime);
    void publishConnectionStatus();

    // Logging and diagnostics
    void logPerformanceMetrics();
    void logComponentStatus();

    // Static instance for callbacks
    static DeviceManager* instance;

    // Configuration
    static const unsigned long DEFAULT_STATUS_INTERVAL = 2000;  // 2 seconds
    static const unsigned long MAX_LOOP_TIME = 100;  // 100ms warning threshold
};

#endif
