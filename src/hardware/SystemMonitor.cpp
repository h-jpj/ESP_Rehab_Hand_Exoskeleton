#include "SystemMonitor.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "../utils/TimeManager.h"

void SystemMonitor::initialize() {
    if (initialized) return;
    
    Logger::info("Initializing System Monitor...");
    
    // Initialize metrics
    currentMetrics = {};
    lastStatusReport = 0;
    statusReportInterval = DEFAULT_STATUS_INTERVAL;
    
    // Initialize thresholds
    memoryThreshold = DEFAULT_MEMORY_THRESHOLD;
    loopTimeThreshold = DEFAULT_LOOP_TIME_THRESHOLD;
    
    // Initialize performance tracking
    totalLoopTime = 0;
    loopCount = 0;
    maxLoopTimeRecorded = 0;
    minFreeHeapRecorded = ESP.getFreeHeap();
    
    // Initialize network status
    wifiConnected = false;
    mqttConnected = false;
    bleConnected = false;
    wifiRssi = -100;
    ipAddress = "";
    
    // Initialize callbacks
    statusCallback = nullptr;
    alertCallback = nullptr;
    
    // Collect initial metrics
    collectMetrics();
    
    initialized = true;
    Logger::info("System Monitor initialized");
    logSystemStatus();
}

void SystemMonitor::update() {
    if (!initialized) return;
    
    collectMetrics();
    checkSystemAlerts();
    
    // Periodic status reporting
    unsigned long now = millis();
    if (now - lastStatusReport >= statusReportInterval) {
        publishStatusUpdate();
        lastStatusReport = now;
    }
}

SystemMetrics SystemMonitor::getSystemMetrics() {
    if (initialized) {
        collectMetrics();
        return currentMetrics;
    }
    
    return {};  // Return empty metrics if not initialized
}

void SystemMonitor::collectMetrics() {
    updateMemoryMetrics();
    updateSystemMetrics();
    updatePerformanceMetrics();
    
    // Assess overall health
    currentMetrics.overallHealth = assessSystemHealth();
    currentMetrics.healthMessage = getHealthMessage();
}

SystemHealth SystemMonitor::assessSystemHealth() {
    // Check memory health
    float memoryUsage = getMemoryUsagePercent();
    if (memoryUsage > MEMORY_CRITICAL_PERCENT) {
        return SystemHealth::CRITICAL;
    } else if (memoryUsage > MEMORY_WARNING_PERCENT) {
        return SystemHealth::WARNING;
    }
    
    // Check performance health
    if (currentMetrics.averageLoopTime > loopTimeThreshold) {
        return SystemHealth::WARNING;
    }
    
    // Check network health
    if (!isNetworkHealthy()) {
        return SystemHealth::WARNING;
    }
    
    // Check for errors
    if (ErrorHandler::hasCriticalErrors()) {
        return SystemHealth::CRITICAL;
    } else if (ErrorHandler::hasErrors()) {
        return SystemHealth::WARNING;
    }
    
    // All checks passed
    if (memoryUsage < 50.0f && currentMetrics.averageLoopTime < 50) {
        return SystemHealth::EXCELLENT;
    } else {
        return SystemHealth::GOOD;
    }
}

String SystemMonitor::getHealthMessage() {
    switch (currentMetrics.overallHealth) {
        case SystemHealth::EXCELLENT:
            return "System running optimally";
        case SystemHealth::GOOD:
            return "System running well";
        case SystemHealth::WARNING:
            return "System has minor issues";
        case SystemHealth::CRITICAL:
            return "System has critical issues";
        default:
            return "System health unknown";
    }
}

bool SystemMonitor::isSystemHealthy() {
    SystemHealth health = assessSystemHealth();
    return (health == SystemHealth::EXCELLENT || health == SystemHealth::GOOD);
}

size_t SystemMonitor::getFreeHeap() {
    return ESP.getFreeHeap();
}

size_t SystemMonitor::getMinFreeHeap() {
    return ESP.getMinFreeHeap();
}

float SystemMonitor::getMemoryUsagePercent() {
    size_t totalHeap = currentMetrics.totalHeap;
    size_t freeHeap = currentMetrics.freeHeap;
    
    if (totalHeap == 0) return 0.0f;
    
    return ((float)(totalHeap - freeHeap) / (float)totalHeap) * 100.0f;
}

bool SystemMonitor::isLowMemory() {
    return currentMetrics.freeHeap < memoryThreshold;
}

void SystemMonitor::recordLoopTime(unsigned long loopTime) {
    totalLoopTime += loopTime;
    loopCount++;
    
    if (loopTime > maxLoopTimeRecorded) {
        maxLoopTimeRecorded = loopTime;
    }
}

unsigned long SystemMonitor::getAverageLoopTime() {
    if (loopCount == 0) return 0;
    return totalLoopTime / loopCount;
}

unsigned long SystemMonitor::getMaxLoopTime() {
    return maxLoopTimeRecorded;
}

unsigned long SystemMonitor::getLoopCount() {
    return loopCount;
}

unsigned long SystemMonitor::getUptime() {
    return TimeManager::getUptime();
}

String SystemMonitor::getUptimeString() {
    unsigned long uptime = getUptime() / 1000;  // Convert to seconds
    
    unsigned long days = uptime / 86400;
    uptime %= 86400;
    unsigned long hours = uptime / 3600;
    uptime %= 3600;
    unsigned long minutes = uptime / 60;
    unsigned long seconds = uptime % 60;
    
    if (days > 0) {
        return String(days) + "d " + String(hours) + "h " + String(minutes) + "m";
    } else if (hours > 0) {
        return String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
    } else if (minutes > 0) {
        return String(minutes) + "m " + String(seconds) + "s";
    } else {
        return String(seconds) + "s";
    }
}

uint32_t SystemMonitor::getCpuFrequency() {
    return ESP.getCpuFreqMHz();
}

String SystemMonitor::getChipModel() {
    return String(ESP.getChipModel());
}

int SystemMonitor::getChipRevision() {
    return ESP.getChipRevision();
}

void SystemMonitor::updateNetworkStatus(bool wifi, bool mqtt, bool ble, int rssi, const String& ip) {
    wifiConnected = wifi;
    mqttConnected = mqtt;
    bleConnected = ble;
    wifiRssi = rssi;
    ipAddress = ip;
    
    // Update metrics
    currentMetrics.wifiConnected = wifi;
    currentMetrics.mqttConnected = mqtt;
    currentMetrics.bleConnected = ble;
    currentMetrics.wifiRssi = rssi;
    currentMetrics.ipAddress = ip;
}

bool SystemMonitor::isNetworkHealthy() {
    // At least WiFi should be connected for basic functionality
    return wifiConnected;
}

void SystemMonitor::publishStatusUpdate() {
    if (statusCallback) {
        statusCallback(currentMetrics);
    }
    
    Logger::debugf("System Status - Health: %s, Memory: %.1f%%, Uptime: %s", 
                  getHealthMessage().c_str(), 
                  getMemoryUsagePercent(), 
                  getUptimeString().c_str());
}

void SystemMonitor::setStatusCallback(void (*callback)(const SystemMetrics& metrics)) {
    statusCallback = callback;
}

void SystemMonitor::checkSystemAlerts() {
    static SystemHealth lastHealth = SystemHealth::UNKNOWN;
    
    SystemHealth currentHealth = assessSystemHealth();
    
    // Check for health changes
    if (currentHealth != lastHealth) {
        if (currentHealth == SystemHealth::CRITICAL || currentHealth == SystemHealth::WARNING) {
            notifyAlert(currentHealth, getHealthMessage());
        }
        lastHealth = currentHealth;
    }
    
    // Check specific conditions
    if (isLowMemory()) {
        logMemoryWarning();
    }
    
    if (currentMetrics.averageLoopTime > loopTimeThreshold) {
        logPerformanceWarning();
    }
}

void SystemMonitor::setAlertCallback(void (*callback)(SystemHealth health, const String& message)) {
    alertCallback = callback;
}

void SystemMonitor::setMemoryThreshold(size_t threshold) {
    memoryThreshold = threshold;
    Logger::infof("Memory threshold set to %d bytes", threshold);
}

void SystemMonitor::setLoopTimeThreshold(unsigned long threshold) {
    loopTimeThreshold = threshold;
    Logger::infof("Loop time threshold set to %lu ms", threshold);
}

void SystemMonitor::setStatusReportInterval(unsigned long interval) {
    statusReportInterval = interval;
    Logger::infof("Status report interval set to %lu ms", interval);
}

void SystemMonitor::updateMemoryMetrics() {
    currentMetrics.freeHeap = ESP.getFreeHeap();
    currentMetrics.totalHeap = ESP.getHeapSize();
    currentMetrics.minFreeHeap = ESP.getMinFreeHeap();
    currentMetrics.maxAllocHeap = ESP.getMaxAllocHeap();
    
    // Update minimum recorded
    if (currentMetrics.freeHeap < minFreeHeapRecorded) {
        minFreeHeapRecorded = currentMetrics.freeHeap;
    }
}

void SystemMonitor::updateSystemMetrics() {
    currentMetrics.uptime = getUptime();
    currentMetrics.cpuFrequency = getCpuFrequency();
    
    // Temperature is not easily available on ESP32, set to 0
    currentMetrics.cpuTemperature = 0.0f;
}

void SystemMonitor::updatePerformanceMetrics() {
    currentMetrics.loopCount = loopCount;
    currentMetrics.averageLoopTime = getAverageLoopTime();
    currentMetrics.maxLoopTime = maxLoopTimeRecorded;
}

void SystemMonitor::logSystemStatus() {
    Logger::infof("System Status:");
    Logger::infof("  Chip: %s Rev %d", getChipModel().c_str(), getChipRevision());
    Logger::infof("  CPU: %d MHz", getCpuFrequency());
    Logger::infof("  Memory: %d bytes free (%.1f%% used)", 
                 currentMetrics.freeHeap, getMemoryUsagePercent());
    Logger::infof("  Uptime: %s", getUptimeString().c_str());
    Logger::infof("  Health: %s", getHealthMessage().c_str());
}

void SystemMonitor::logMemoryWarning() {
    static unsigned long lastWarning = 0;
    unsigned long now = millis();
    
    // Limit warning frequency to once per minute
    if (now - lastWarning > 60000) {
        Logger::warningf("Low memory warning: %d bytes free (threshold: %d)", 
                        currentMetrics.freeHeap, memoryThreshold);
        REPORT_WARNING(ErrorCode::LOW_MEMORY, "System memory running low");
        lastWarning = now;
    }
}

void SystemMonitor::logPerformanceWarning() {
    static unsigned long lastWarning = 0;
    unsigned long now = millis();
    
    // Limit warning frequency to once per minute
    if (now - lastWarning > 60000) {
        Logger::warningf("Performance warning: Average loop time %lu ms (threshold: %lu)", 
                        currentMetrics.averageLoopTime, loopTimeThreshold);
        REPORT_WARNING(ErrorCode::SYSTEM_OVERLOAD, "System performance degraded");
        lastWarning = now;
    }
}

void SystemMonitor::notifyAlert(SystemHealth health, const String& message) {
    if (alertCallback) {
        alertCallback(health, message);
    }
    
    Logger::warningf("System Alert: %s", message.c_str());
}
