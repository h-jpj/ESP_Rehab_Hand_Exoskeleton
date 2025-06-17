#include "SystemHealthManager.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"

void SystemHealthManager::initialize() {
    if (initialized) return;
    
    Logger::info("Initializing System Health Manager...");
    
    currentHealth = HealthStatus::GOOD;
    startTime = millis();
    alertCount = 0;
    lastAlert = "";
    newAlertsAvailable = false;
    totalLoopTime = 0;
    loopCount = 0;
    maxLoopTime = 0;
    lastHealthCheck = 0;
    lastFreeHeap = ESP.getFreeHeap();
    minFreeHeapEver = lastFreeHeap;
    memoryLeakCount = 0;
    taskHandle = nullptr;
    taskRunning = false;
    healthCallback = nullptr;
    
    initialized = true;
    
    // Start the system health monitoring task
    startTask();
    
    Logger::info("System Health Manager initialized with FreeRTOS task");
}

void SystemHealthManager::shutdown() {
    if (!initialized) return;
    
    Logger::info("Shutting down System Health Manager...");
    
    stopTask();
    
    initialized = false;
    Logger::info("System Health Manager shutdown complete");
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void SystemHealthManager::startTask() {
    if (taskRunning || taskHandle) return;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        systemHealthTask,
        "SystemHealth",
        TASK_STACK_SYSTEM_HEALTH,
        this,
        PRIORITY_SYSTEM_HEALTH,
        &taskHandle,
        CORE_APPLICATION  // Core 1 for application tasks
    );
    
    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("System Health task started on Core 1");
    } else {
        Logger::error("Failed to create System Health task");
    }
}

void SystemHealthManager::stopTask() {
    if (!taskRunning || !taskHandle) return;
    
    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
    
    Logger::info("System Health task stopped");
}

bool SystemHealthManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void SystemHealthManager::systemHealthTask(void* parameter) {
    SystemHealthManager* manager = (SystemHealthManager*)parameter;
    Logger::info("System Health task started");
    
    while (manager->taskRunning) {
        // Update system health metrics
        manager->updateMemoryMetrics();
        manager->updateTaskMetrics();
        manager->checkSystemHealth();
        manager->detectMemoryLeaks();
        manager->monitorTaskPerformance();
        
        // Process any system alerts
        manager->processSystemAlerts();
        
        // Log health report periodically (every 30 seconds)
        uint32_t now = millis();
        if (now - manager->lastHealthCheck >= 30000) {
            manager->logHealthReport();
            manager->lastHealthCheck = now;
        }
        
        // Feed watchdog (when FreeRTOS Manager is re-enabled)
        // FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());
        
        // Health monitoring runs every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(HEALTH_CHECK_INTERVAL));
    }
    
    Logger::info("System Health task ended");
    vTaskDelete(nullptr);  // Delete self
}

// =============================================================================
// HEALTH MONITORING
// =============================================================================

HealthStatus SystemHealthManager::getOverallHealth() {
    return currentHealth;
}

MemoryMetrics SystemHealthManager::getMemoryMetrics() {
    MemoryMetrics metrics;
    
    metrics.totalHeap = ESP.getHeapSize();
    metrics.freeHeap = ESP.getFreeHeap();
    metrics.minFreeHeap = ESP.getMinFreeHeap();
    metrics.largestFreeBlock = ESP.getMaxAllocHeap();
    
    metrics.usagePercent = ((float)(metrics.totalHeap - metrics.freeHeap) / metrics.totalHeap) * 100.0f;
    metrics.fragmentationPercent = ((float)(metrics.freeHeap - metrics.largestFreeBlock) / metrics.freeHeap) * 100.0f;
    
    return metrics;
}

SystemHealthReport SystemHealthManager::getHealthReport() {
    SystemHealthReport report;
    
    report.overallHealth = currentHealth;
    report.memory = getMemoryMetrics();
    report.uptime = getUptime();
    report.totalTasks = uxTaskGetNumberOfTasks();
    report.runningTasks = 0; // Will be calculated in updateTaskMetrics
    report.averageCpuUsage = getCpuUsage();
    report.systemAlerts = alertCount;
    report.lastAlert = lastAlert;
    
    return report;
}

// =============================================================================
// PERFORMANCE MONITORING
// =============================================================================

void SystemHealthManager::recordLoopTime(uint32_t loopTime) {
    totalLoopTime += loopTime;
    loopCount++;
    
    if (loopTime > maxLoopTime) {
        maxLoopTime = loopTime;
    }
    
    // Check for performance warnings
    if (loopTime > MAX_LOOP_TIME_CRITICAL) {
        reportAlert("Critical loop time: " + String(loopTime) + "ms", HealthStatus::CRITICAL);
    } else if (loopTime > MAX_LOOP_TIME_WARNING) {
        reportAlert("High loop time: " + String(loopTime) + "ms", HealthStatus::WARNING);
    }
}

uint32_t SystemHealthManager::getAverageLoopTime() {
    if (loopCount == 0) return 0;
    return totalLoopTime / loopCount;
}

uint32_t SystemHealthManager::getMaxLoopTime() {
    return maxLoopTime;
}

uint32_t SystemHealthManager::getUptime() {
    return (millis() - startTime) / 1000; // Return in seconds
}

float SystemHealthManager::getCpuUsage() {
    // Simplified CPU usage calculation
    // In a real implementation, this would use FreeRTOS runtime stats
    uint32_t avgLoopTime = getAverageLoopTime();
    if (avgLoopTime == 0) return 0.0f;
    
    // Estimate CPU usage based on loop performance
    return constrain((float)avgLoopTime / 10.0f, 0.0f, 100.0f);
}

bool SystemHealthManager::isSystemHealthy() {
    return currentHealth == HealthStatus::EXCELLENT || currentHealth == HealthStatus::GOOD;
}

// =============================================================================
// ALERT SYSTEM
// =============================================================================

void SystemHealthManager::reportAlert(const String& alert, HealthStatus severity) {
    alertCount++;
    lastAlert = alert;
    newAlertsAvailable = true;
    
    Logger::warningf("System Health Alert [%d]: %s", (int)severity, alert.c_str());
    
    // Update health status if this alert is more severe
    if (severity > currentHealth) {
        currentHealth = severity;
    }
    
    // Notify via callback if set
    if (healthCallback) {
        healthCallback(severity, alert);
    }
}

bool SystemHealthManager::hasNewAlerts() {
    return newAlertsAvailable;
}

String SystemHealthManager::getLastAlert() {
    return lastAlert;
}

void SystemHealthManager::clearAlerts() {
    newAlertsAvailable = false;
}

void SystemHealthManager::setHealthCallback(void (*callback)(HealthStatus status, const String& message)) {
    healthCallback = callback;
}

// =============================================================================
// INTERNAL MONITORING METHODS
// =============================================================================

void SystemHealthManager::updateMemoryMetrics() {
    size_t currentFreeHeap = ESP.getFreeHeap();

    // Track minimum free heap
    if (currentFreeHeap < minFreeHeapEver) {
        minFreeHeapEver = currentFreeHeap;
    }

    // Update last free heap for leak detection
    lastFreeHeap = currentFreeHeap;
}

void SystemHealthManager::updateTaskMetrics() {
    // This would be implemented with FreeRTOS runtime stats
    // For now, just count total tasks
    uint32_t taskCount = uxTaskGetNumberOfTasks();

    if (taskCount > 20) {  // Arbitrary threshold
        reportAlert("High task count: " + String(taskCount), HealthStatus::WARNING);
    }
}

void SystemHealthManager::checkSystemHealth() {
    currentHealth = assessOverallHealth();
}

void SystemHealthManager::detectMemoryLeaks() {
    MemoryMetrics metrics = getMemoryMetrics();

    // Check for memory usage warnings
    if (metrics.usagePercent > MEMORY_CRITICAL_THRESHOLD) {
        reportAlert("Critical memory usage: " + String(metrics.usagePercent, 1) + "%", HealthStatus::CRITICAL);
    } else if (metrics.usagePercent > MEMORY_WARNING_THRESHOLD) {
        reportAlert("High memory usage: " + String(metrics.usagePercent, 1) + "%", HealthStatus::WARNING);
    }

    // Check for memory fragmentation
    if (metrics.fragmentationPercent > 50.0f) {
        reportAlert("High memory fragmentation: " + String(metrics.fragmentationPercent, 1) + "%", HealthStatus::WARNING);
    }
}

void SystemHealthManager::monitorTaskPerformance() {
    // Monitor overall system performance
    uint32_t avgLoopTime = getAverageLoopTime();

    if (avgLoopTime > MAX_LOOP_TIME_WARNING) {
        reportAlert("System performance degraded: " + String(avgLoopTime) + "ms avg loop", HealthStatus::WARNING);
    }
}

HealthStatus SystemHealthManager::assessOverallHealth() {
    HealthStatus memoryHealth = assessMemoryHealth();
    HealthStatus taskHealth = assessTaskHealth();

    // Return the worst health status
    return (memoryHealth > taskHealth) ? memoryHealth : taskHealth;
}

HealthStatus SystemHealthManager::assessMemoryHealth() {
    MemoryMetrics metrics = getMemoryMetrics();

    if (metrics.usagePercent > MEMORY_CRITICAL_THRESHOLD) {
        return HealthStatus::CRITICAL;
    } else if (metrics.usagePercent > MEMORY_WARNING_THRESHOLD) {
        return HealthStatus::WARNING;
    } else if (metrics.usagePercent > 60.0f) {
        return HealthStatus::GOOD;
    } else {
        return HealthStatus::EXCELLENT;
    }
}

HealthStatus SystemHealthManager::assessTaskHealth() {
    uint32_t avgLoopTime = getAverageLoopTime();

    if (avgLoopTime > MAX_LOOP_TIME_CRITICAL) {
        return HealthStatus::CRITICAL;
    } else if (avgLoopTime > MAX_LOOP_TIME_WARNING) {
        return HealthStatus::WARNING;
    } else if (avgLoopTime > 50) {
        return HealthStatus::GOOD;
    } else {
        return HealthStatus::EXCELLENT;
    }
}

void SystemHealthManager::processSystemAlerts() {
    // Process any pending system alerts
    // This could integrate with FreeRTOS Manager alert queue when re-enabled
}

void SystemHealthManager::logHealthReport() {
    SystemHealthReport report = getHealthReport();

    Logger::info("=== System Health Report ===");
    Logger::infof("Overall Health: %d", (int)report.overallHealth);
    Logger::infof("Uptime: %lu seconds", report.uptime);
    Logger::infof("Memory Usage: %.1f%% (%zu/%zu bytes)",
                 report.memory.usagePercent,
                 report.memory.totalHeap - report.memory.freeHeap,
                 report.memory.totalHeap);
    Logger::infof("Free Heap: %zu bytes (min: %zu)", report.memory.freeHeap, report.memory.minFreeHeap);
    Logger::infof("Tasks: %lu total", report.totalTasks);
    Logger::infof("Average Loop Time: %lu ms (max: %lu ms)", getAverageLoopTime(), getMaxLoopTime());

    if (report.systemAlerts > 0) {
        Logger::infof("System Alerts: %lu (last: %s)", report.systemAlerts, report.lastAlert.c_str());
    }
}
