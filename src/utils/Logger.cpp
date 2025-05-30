#include "Logger.h"
#include <stdarg.h>

// Static member initialization
LogLevel Logger::currentLevel = LogLevel::INFO;
bool Logger::initialized = false;

void Logger::initialize(LogLevel level) {
    if (!initialized) {
        Serial.begin(115200);
        while (!Serial) {
            delay(10);
        }
        currentLevel = level;
        initialized = true;
        
        info("Logger initialized");
        logSystemInfo();
    }
}

void Logger::debug(const String& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const String& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const String& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const String& message) {
    log(LogLevel::ERROR, message);
}

void Logger::debugf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::DEBUG, format, args);
    va_end(args);
}

void Logger::infof(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::INFO, format, args);
    va_end(args);
}

void Logger::warningf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::WARNING, format, args);
    va_end(args);
}

void Logger::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::ERROR, format, args);
    va_end(args);
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
    infof("Log level set to: %s", getLevelString(level));
}

LogLevel Logger::getLevel() {
    return currentLevel;
}

void Logger::logSystemInfo() {
    infof("ESP32 Chip Model: %s", ESP.getChipModel());
    infof("Chip Revision: %d", ESP.getChipRevision());
    infof("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
    infof("Flash Size: %d bytes", ESP.getFlashChipSize());
    logMemoryUsage();
}

void Logger::logMemoryUsage() {
    infof("Free Heap: %d bytes", ESP.getFreeHeap());
    infof("Largest Free Block: %d bytes", ESP.getMaxAllocHeap());
    infof("Min Free Heap: %d bytes", ESP.getMinFreeHeap());
}

void Logger::log(LogLevel level, const String& message) {
    if (level >= currentLevel && initialized) {
        String timestamp = getTimestamp();
        String levelStr = getLevelString(level);
        
        Serial.printf("[%s] %s: %s\n", 
                     timestamp.c_str(), 
                     levelStr, 
                     message.c_str());
    }
}

void Logger::logf(LogLevel level, const char* format, va_list args) {
    if (level >= currentLevel && initialized) {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        log(level, String(buffer));
    }
}

const char* Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKN ";
    }
}

String Logger::getTimestamp() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    ms %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu.%03lu", 
             hours, minutes, seconds, ms);
    
    return String(buffer);
}
