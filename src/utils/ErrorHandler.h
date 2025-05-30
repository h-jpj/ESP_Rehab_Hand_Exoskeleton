#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>

enum class ErrorCode {
    NONE = 0,
    WIFI_CONNECTION_FAILED = 1,
    MQTT_CONNECTION_FAILED = 2,
    BLE_INITIALIZATION_FAILED = 3,
    SERVO_INITIALIZATION_FAILED = 4,
    LOW_MEMORY = 5,
    NTP_SYNC_FAILED = 6,
    INVALID_COMMAND = 7,
    SYSTEM_OVERLOAD = 8,
    HARDWARE_FAULT = 9
};

enum class ErrorSeverity {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3
};

struct ErrorInfo {
    ErrorCode code;
    ErrorSeverity severity;
    String message;
    unsigned long timestamp;
    String component;
};

class ErrorHandler {
public:
    static void initialize();
    
    // Error reporting
    static void reportError(ErrorCode code, const String& message, const String& component = "");
    static void reportWarning(ErrorCode code, const String& message, const String& component = "");
    static void reportCritical(ErrorCode code, const String& message, const String& component = "");
    
    // Error queries
    static bool hasErrors();
    static bool hasCriticalErrors();
    static ErrorInfo getLastError();
    static int getErrorCount();
    static void clearErrors();
    
    // Recovery actions
    static bool attemptRecovery(ErrorCode code);
    static void resetSystem();
    
    // Status
    static String getSystemHealthStatus();
    static void logErrorSummary();

private:
    static const int MAX_ERRORS = 10;
    static ErrorInfo errorHistory[MAX_ERRORS];
    static int errorCount;
    static int errorIndex;
    static bool initialized;
    
    static void addError(ErrorCode code, ErrorSeverity severity, const String& message, const String& component);
    static const char* getErrorCodeString(ErrorCode code);
    static const char* getSeverityString(ErrorSeverity severity);
    static void performRecoveryAction(ErrorCode code);
};

// Convenience macros
#define REPORT_ERROR(code, msg) ErrorHandler::reportError(code, msg, __FUNCTION__)
#define REPORT_WARNING(code, msg) ErrorHandler::reportWarning(code, msg, __FUNCTION__)
#define REPORT_CRITICAL(code, msg) ErrorHandler::reportCritical(code, msg, __FUNCTION__)

#endif
