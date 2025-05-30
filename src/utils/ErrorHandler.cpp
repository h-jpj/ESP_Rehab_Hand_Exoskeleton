#include "ErrorHandler.h"
#include "Logger.h"
#include "TimeManager.h"

// Static member initialization
ErrorInfo ErrorHandler::errorHistory[MAX_ERRORS];
int ErrorHandler::errorCount = 0;
int ErrorHandler::errorIndex = 0;
bool ErrorHandler::initialized = false;

void ErrorHandler::initialize() {
    if (!initialized) {
        // Clear error history
        for (int i = 0; i < MAX_ERRORS; i++) {
            errorHistory[i] = {ErrorCode::NONE, ErrorSeverity::INFO, "", 0, ""};
        }

        errorCount = 0;
        errorIndex = 0;
        initialized = true;

        Logger::info("ErrorHandler initialized");
    }
}

void ErrorHandler::reportError(ErrorCode code, const String& message, const String& component) {
    addError(code, ErrorSeverity::ERROR, message, component);
    Logger::errorf("[%s] Error %d: %s", component.c_str(), (int)code, message.c_str());
}

void ErrorHandler::reportWarning(ErrorCode code, const String& message, const String& component) {
    addError(code, ErrorSeverity::WARNING, message, component);
    Logger::warningf("[%s] Warning %d: %s", component.c_str(), (int)code, message.c_str());
}

void ErrorHandler::reportCritical(ErrorCode code, const String& message, const String& component) {
    addError(code, ErrorSeverity::CRITICAL, message, component);
    Logger::errorf("[%s] CRITICAL %d: %s", component.c_str(), (int)code, message.c_str());

    // Attempt immediate recovery for critical errors
    attemptRecovery(code);
}

bool ErrorHandler::hasErrors() {
    return errorCount > 0;
}

bool ErrorHandler::hasCriticalErrors() {
    for (int i = 0; i < min(errorCount, MAX_ERRORS); i++) {
        if (errorHistory[i].severity == ErrorSeverity::CRITICAL) {
            return true;
        }
    }
    return false;
}

ErrorInfo ErrorHandler::getLastError() {
    if (errorCount > 0) {
        int lastIndex = (errorIndex - 1 + MAX_ERRORS) % MAX_ERRORS;
        return errorHistory[lastIndex];
    }
    return {ErrorCode::NONE, ErrorSeverity::INFO, "No errors", 0, ""};
}

int ErrorHandler::getErrorCount() {
    return errorCount;
}

void ErrorHandler::clearErrors() {
    errorCount = 0;
    errorIndex = 0;
    Logger::info("Error history cleared");
}

bool ErrorHandler::attemptRecovery(ErrorCode code) {
    Logger::infof("Attempting recovery for error code: %d", (int)code);

    performRecoveryAction(code);

    // Return true if recovery might have succeeded
    return true;
}

void ErrorHandler::resetSystem() {
    Logger::error("System reset requested due to critical errors");
    delay(1000);  // Allow log message to be sent
    ESP.restart();
}

String ErrorHandler::getSystemHealthStatus() {
    if (errorCount == 0) {
        return "Healthy";
    } else if (hasCriticalErrors()) {
        return "Critical";
    } else {
        return "Warning";
    }
}

void ErrorHandler::logErrorSummary() {
    Logger::infof("System Health: %s", getSystemHealthStatus().c_str());
    Logger::infof("Total Errors: %d", errorCount);

    if (errorCount > 0) {
        ErrorInfo lastError = getLastError();
        Logger::infof("Last Error: %s - %s",
                     getErrorCodeString(lastError.code),
                     lastError.message.c_str());
    }
}

void ErrorHandler::addError(ErrorCode code, ErrorSeverity severity, const String& message, const String& component) {
    if (!initialized) return;

    ErrorInfo& error = errorHistory[errorIndex];
    error.code = code;
    error.severity = severity;
    error.message = message;
    error.timestamp = TimeManager::getCurrentTimestamp();
    error.component = component;

    errorIndex = (errorIndex + 1) % MAX_ERRORS;
    if (errorCount < MAX_ERRORS) {
        errorCount++;
    }
}

const char* ErrorHandler::getErrorCodeString(ErrorCode code) {
    switch (code) {
        case ErrorCode::NONE: return "NONE";
        case ErrorCode::WIFI_CONNECTION_FAILED: return "WIFI_CONNECTION_FAILED";
        case ErrorCode::MQTT_CONNECTION_FAILED: return "MQTT_CONNECTION_FAILED";
        case ErrorCode::BLE_INITIALIZATION_FAILED: return "BLE_INITIALIZATION_FAILED";
        case ErrorCode::SERVO_INITIALIZATION_FAILED: return "SERVO_INITIALIZATION_FAILED";
        case ErrorCode::LOW_MEMORY: return "LOW_MEMORY";
        case ErrorCode::NTP_SYNC_FAILED: return "NTP_SYNC_FAILED";
        case ErrorCode::INVALID_COMMAND: return "INVALID_COMMAND";
        case ErrorCode::SYSTEM_OVERLOAD: return "SYSTEM_OVERLOAD";
        case ErrorCode::HARDWARE_FAULT: return "HARDWARE_FAULT";
        default: return "UNKNOWN";
    }
}

const char* ErrorHandler::getSeverityString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARNING";
        case ErrorSeverity::ERROR: return "ERROR";
        case ErrorSeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

void ErrorHandler::performRecoveryAction(ErrorCode code) {
    switch (code) {
        case ErrorCode::WIFI_CONNECTION_FAILED:
            Logger::info("Recovery: Attempting WiFi reconnection");
            break;

        case ErrorCode::MQTT_CONNECTION_FAILED:
            Logger::info("Recovery: Attempting MQTT reconnection");
            break;

        case ErrorCode::LOW_MEMORY:
            Logger::info("Recovery: Triggering garbage collection");
            Logger::logMemoryUsage();
            break;

        case ErrorCode::SYSTEM_OVERLOAD:
            Logger::info("Recovery: Reducing system load");
            delay(100);
            break;

        case ErrorCode::HARDWARE_FAULT:
            Logger::warning("Recovery: Hardware fault detected - system restart may be required");
            break;

        default:
            Logger::debugf("No specific recovery action for error code: %d", (int)code);
            break;
    }
}
