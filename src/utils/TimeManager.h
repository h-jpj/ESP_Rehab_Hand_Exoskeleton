#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

class TimeManager {
public:
    // Initialization
    static void initialize();
    static void syncWithNTP();
    
    // Time queries
    static unsigned long getCurrentTimestamp();
    static bool isTimeValid();
    static String getCurrentTimeString();
    static unsigned long getUptime();
    
    // Status
    static bool isNTPSynced();
    static String getLastSyncTime();
    
private:
    static bool initialized;
    static bool ntpSynced;
    static unsigned long bootTime;
    static unsigned long lastSyncTime;
    
    static const char* NTP_SERVER1;
    static const char* NTP_SERVER2;
    static const long GMT_OFFSET_SEC;
    static const int DAYLIGHT_OFFSET_SEC;
    
    static bool attemptNTPSync();
    static void logTimeStatus();
};

#endif
