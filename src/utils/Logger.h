#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static void initialize(LogLevel level = LogLevel::INFO);
    
    // Logging methods
    static void debug(const String& message);
    static void info(const String& message);
    static void warning(const String& message);
    static void error(const String& message);
    
    // Formatted logging
    static void debugf(const char* format, ...);
    static void infof(const char* format, ...);
    static void warningf(const char* format, ...);
    static void errorf(const char* format, ...);
    
    // Configuration
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    
    // System info logging
    static void logSystemInfo();
    static void logMemoryUsage();

private:
    static LogLevel currentLevel;
    static bool initialized;
    
    static void log(LogLevel level, const String& message);
    static void logf(LogLevel level, const char* format, va_list args);
    static const char* getLevelString(LogLevel level);
    static String getTimestamp();
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARNING(msg) Logger::warning(msg)
#define LOG_ERROR(msg) Logger::error(msg)

#define LOG_DEBUGF(fmt, ...) Logger::debugf(fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) Logger::infof(fmt, ##__VA_ARGS__)
#define LOG_WARNINGF(fmt, ...) Logger::warningf(fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...) Logger::errorf(fmt, ##__VA_ARGS__)

#endif
