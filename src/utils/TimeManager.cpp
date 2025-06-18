#include "TimeManager.h"
#include "Logger.h"

// Static member initialization
bool TimeManager::initialized = false;
bool TimeManager::ntpSynced = false;
unsigned long TimeManager::bootTime = 0;
unsigned long TimeManager::lastSyncTime = 0;

const char* TimeManager::NTP_SERVER1 = "pool.ntp.org";
const char* TimeManager::NTP_SERVER2 = "time.nist.gov";
const long TimeManager::GMT_OFFSET_SEC = 0;  // UTC
const int TimeManager::DAYLIGHT_OFFSET_SEC = 0;

void TimeManager::initialize() {
    if (!initialized) {
        bootTime = millis();
        initialized = true;
        Logger::info("TimeManager initialized");
        
        // Will sync when WiFi is available
        if (WiFi.status() == WL_CONNECTED) {
            syncWithNTP();
        }
    }
}

void TimeManager::syncWithNTP() {
    if (!WiFi.isConnected()) {
        Logger::warning("Cannot sync NTP - WiFi not connected");
        return;
    }
    
    Logger::info("Attempting NTP time synchronization...");
    
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    
    if (attemptNTPSync()) {
        ntpSynced = true;
        lastSyncTime = millis();
        Logger::info("NTP time synchronization successful");
        logTimeStatus();
    } else {
        Logger::warning("NTP time synchronization failed");
    }
}

unsigned long TimeManager::getCurrentTimestamp() {
    time_t now = time(nullptr);
    
    if (isTimeValid()) {
        return now;
    } else {
        // Fallback: Use a recent timestamp + uptime
        // May 30, 2025 05:00:00 UTC = 1748494800
        unsigned long timestamp = 1748494800 + (getUptime() / 1000);
        return timestamp;
    }
}

bool TimeManager::isTimeValid() {
    time_t now = time(nullptr);
    // Check if time is after Jan 1, 2022 (reasonable minimum)
    return (now > 1640995200);
}

String TimeManager::getCurrentTimeString() {
    time_t now = time(nullptr);
    
    if (isTimeValid()) {
        struct tm* timeinfo = localtime(&now);
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", timeinfo);
        return String(buffer);
    } else {
        return "Time not synchronized";
    }
}

unsigned long TimeManager::getUptime() {
    return millis() - bootTime;
}

bool TimeManager::isNTPSynced() {
    return ntpSynced && isTimeValid();
}

String TimeManager::getLastSyncTime() {
    if (lastSyncTime == 0) {
        return "Never";
    }
    
    unsigned long syncAgo = (millis() - lastSyncTime) / 1000;
    
    if (syncAgo < 60) {
        return String(syncAgo) + " seconds ago";
    } else if (syncAgo < 3600) {
        return String(syncAgo / 60) + " minutes ago";
    } else {
        return String(syncAgo / 3600) + " hours ago";
    }
}

bool TimeManager::attemptNTPSync() {
    const int maxAttempts = 20;
    const int delayMs = 500;
    
    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        time_t now = time(nullptr);
        
        if (now > 1640995200) {  // After Jan 1, 2022
            return true;
        }
        
        vTaskDelay(pdMS_TO_TICKS(delayMs));  // FreeRTOS delay
        
        if (attempt % 5 == 0) {
            Logger::debugf("NTP sync attempt %d/%d", attempt + 1, maxAttempts);
        }
    }
    
    return false;
}

void TimeManager::logTimeStatus() {
    Logger::infof("Current time: %s", getCurrentTimeString().c_str());
    Logger::infof("Uptime: %lu seconds", getUptime() / 1000);
    Logger::infof("NTP synced: %s", isNTPSynced() ? "Yes" : "No");
    Logger::infof("Last sync: %s", getLastSyncTime().c_str());
}
