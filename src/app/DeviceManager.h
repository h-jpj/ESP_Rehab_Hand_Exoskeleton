#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>

// Include all component managers
#include "../network/WiFiManager.h"
#include "../network/MQTTManager.h"

#include "../bluetooth/BLEManager.h"
#include "../hardware/ServoController.h"
#include "../hardware/SystemMonitor.h"
#include "../hardware/FreeRTOSManager.h"
#include "../hardware/I2CManager.h"
// SystemHealthManager removed - using SystemMonitor instead
#include "../analytics/SessionAnalyticsManager.h"
#include "../sensors/PulseMonitorManager.h"
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
    void update();  // Legacy - will be replaced by FreeRTOS task
    void shutdown();

    // FreeRTOS Task Management
    void startTask();
    void stopTask();
    bool isTaskRunning();

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
    // SystemHealthManager removed - using SystemMonitor instead
    SessionAnalyticsManager& getSessionAnalyticsManager();
    PulseMonitorManager& getPulseMonitorManager();
    CommandProcessor& getCommandProcessor();
    SessionManager& getSessionManager();

    // FreeRTOS system access
    static bool isFreeRTOSReady();
    static void logFreeRTOSStatus();

    // Command handling
    bool handleCommand(const String& command, CommandSource source);
    void processQueuedCommands();

    // Status reporting
    void publishSystemStatus();
    void logSystemSummary();
    void logComponentStatus();

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
    // SystemHealthManager removed - using SystemMonitor instead
    SessionAnalyticsManager sessionAnalyticsManager;
    PulseMonitorManager pulseMonitorManager;
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

    // FreeRTOS Task Management
    TaskHandle_t taskHandle;
    bool taskRunning;
    static void deviceManagerTask(void* parameter);

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
    static void onPulseReading(const HeartRateReading& reading);

    // State management
    void setState(DeviceState newState);
    void checkSystemHealth();
    void handleSystemError();

    // Status publishing
    void publishMovementStatus(const Command& command, unsigned long responseTime);
    void publishConnectionStatus();
    void publishServoAnalytics();
    void publishSystemHealthData();

    // Logging and diagnostics
    void logPerformanceMetrics();

    // Static instance for callbacks
    static DeviceManager* instance;

    // Configuration
    static const unsigned long DEFAULT_STATUS_INTERVAL = 2000;  // 2 seconds
    static const unsigned long MAX_LOOP_TIME = 100;  // 100ms warning threshold
};

#endif
