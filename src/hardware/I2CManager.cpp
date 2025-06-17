#include "I2CManager.h"

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

bool I2CManager::initialized = false;
bool I2CManager::taskRunning = false;
TaskHandle_t I2CManager::taskHandle = nullptr;
uint32_t I2CManager::transactionCount = 0;
uint32_t I2CManager::errorCount = 0;
uint32_t I2CManager::lastScanTime = 0;
uint8_t I2CManager::connectedDevices[128] = {0};
uint8_t I2CManager::connectedDeviceCount = 0;
bool I2CManager::busRecoveryInProgress = false;
uint32_t I2CManager::lastRecoveryTime = 0;
uint8_t I2CManager::recoveryAttempts = 0;

// =============================================================================
// PUBLIC METHODS
// =============================================================================

bool I2CManager::initialize() {
    if (initialized) {
        Logger::warning("I2C Manager already initialized");
        return true;
    }

    Logger::info("Initializing I2C Manager...");

    // Initialize I2C bus
    if (!initializeBus()) {
        Logger::error("Failed to initialize I2C bus");
        return false;
    }

    // Reset statistics
    transactionCount = 0;
    errorCount = 0;
    connectedDeviceCount = 0;
    memset(connectedDevices, 0, sizeof(connectedDevices));

    // Scan for connected devices
    scanForDevices();

    // Start the I2C manager task
    startTask();

    initialized = true;
    Logger::info("I2C Manager initialized successfully");
    logConnectedDevices();

    return true;
}

void I2CManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down I2C Manager...");

    stopTask();
    Wire.end();

    initialized = false;
    Logger::info("I2C Manager shutdown complete");
}

bool I2CManager::isInitialized() {
    return initialized;
}

bool I2CManager::writeRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value) {
    if (!initialized) return false;

    uint8_t data[2] = {registerAddress, value};
    
    I2CRequest request;
    request.deviceAddress = deviceAddress;
    request.writeData = data;
    request.writeLength = 2;
    request.readBuffer = nullptr;
    request.readLength = 0;
    request.completionSemaphore = xSemaphoreCreateBinary();
    request.timeout = pdMS_TO_TICKS(I2C_TIMEOUT_MS);
    request.requestId = millis();

    bool success = false;
    request.successFlag = &success;

    if (queueRequest(request)) {
        // Wait for completion
        if (xSemaphoreTake(request.completionSemaphore, request.timeout) == pdTRUE) {
            vSemaphoreDelete(request.completionSemaphore);
            return success;
        }
    }

    vSemaphoreDelete(request.completionSemaphore);
    return false;
}

bool I2CManager::writeRegister16(uint8_t deviceAddress, uint8_t registerAddress, uint16_t value) {
    if (!initialized) return false;

    uint8_t data[3] = {registerAddress, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    
    I2CRequest request;
    request.deviceAddress = deviceAddress;
    request.writeData = data;
    request.writeLength = 3;
    request.readBuffer = nullptr;
    request.readLength = 0;
    request.completionSemaphore = xSemaphoreCreateBinary();
    request.timeout = pdMS_TO_TICKS(I2C_TIMEOUT_MS);
    request.requestId = millis();

    bool success = false;
    request.successFlag = &success;

    if (queueRequest(request)) {
        if (xSemaphoreTake(request.completionSemaphore, request.timeout) == pdTRUE) {
            vSemaphoreDelete(request.completionSemaphore);
            return success;
        }
    }

    vSemaphoreDelete(request.completionSemaphore);
    return false;
}

bool I2CManager::readRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t* value) {
    if (!initialized || !value) return false;

    uint8_t writeData = registerAddress;
    
    I2CRequest request;
    request.deviceAddress = deviceAddress;
    request.writeData = &writeData;
    request.writeLength = 1;
    request.readBuffer = value;
    request.readLength = 1;
    request.completionSemaphore = xSemaphoreCreateBinary();
    request.timeout = pdMS_TO_TICKS(I2C_TIMEOUT_MS);
    request.requestId = millis();

    bool success = false;
    request.successFlag = &success;

    if (queueRequest(request)) {
        if (xSemaphoreTake(request.completionSemaphore, request.timeout) == pdTRUE) {
            vSemaphoreDelete(request.completionSemaphore);
            return success;
        }
    }

    vSemaphoreDelete(request.completionSemaphore);
    return false;
}

bool I2CManager::readRegister16(uint8_t deviceAddress, uint8_t registerAddress, uint16_t* value) {
    if (!initialized || !value) return false;

    uint8_t writeData = registerAddress;
    uint8_t readData[2];
    
    I2CRequest request;
    request.deviceAddress = deviceAddress;
    request.writeData = &writeData;
    request.writeLength = 1;
    request.readBuffer = readData;
    request.readLength = 2;
    request.completionSemaphore = xSemaphoreCreateBinary();
    request.timeout = pdMS_TO_TICKS(I2C_TIMEOUT_MS);
    request.requestId = millis();

    bool success = false;
    request.successFlag = &success;

    if (queueRequest(request)) {
        if (xSemaphoreTake(request.completionSemaphore, request.timeout) == pdTRUE) {
            if (success) {
                *value = (readData[0] << 8) | readData[1];
            }
            vSemaphoreDelete(request.completionSemaphore);
            return success;
        }
    }

    vSemaphoreDelete(request.completionSemaphore);
    return false;
}

bool I2CManager::scanDevices() {
    if (!initialized) return false;

    Logger::info("Scanning I2C bus for devices...");
    scanForDevices();
    logConnectedDevices();
    return true;
}

bool I2CManager::isDevicePresent(uint8_t deviceAddress) {
    if (deviceAddress >= 128) return false;
    return (connectedDevices[deviceAddress / 8] & (1 << (deviceAddress % 8))) != 0;
}

void I2CManager::logConnectedDevices() {
    Logger::infof("I2C devices found: %d", connectedDeviceCount);
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (isDevicePresent(addr)) {
            Logger::infof("  Device at address 0x%02X", addr);
        }
    }
}

bool I2CManager::isBusHealthy() {
    if (transactionCount == 0) return true;
    
    float successRate = getSuccessRate();
    bool healthy = successRate > 0.95f;  // 95% success rate threshold
    
    // Check if recovery is needed
    if (!healthy && !busRecoveryInProgress) {
        Logger::warningf("I2C bus health poor: %.1f%% success rate", successRate * 100);
    }
    
    return healthy;
}

uint32_t I2CManager::getTransactionCount() {
    return transactionCount;
}

uint32_t I2CManager::getErrorCount() {
    return errorCount;
}

float I2CManager::getSuccessRate() {
    if (transactionCount == 0) return 1.0f;
    return (float)(transactionCount - errorCount) / transactionCount;
}

void I2CManager::startTask() {
    if (taskRunning) return;

    BaseType_t result = xTaskCreatePinnedToCore(
        i2cManagerTask,
        "I2CManager",
        TASK_STACK_I2C_MANAGER,
        nullptr,
        PRIORITY_I2C_MANAGER,
        &taskHandle,
        CORE_APPLICATION
    );

    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("I2C Manager task started on core 1");
    } else {
        Logger::error("Failed to create I2C Manager task");
    }
}

void I2CManager::stopTask() {
    if (!taskRunning || !taskHandle) return;

    vTaskDelete(taskHandle);
    taskHandle = nullptr;
    taskRunning = false;
    Logger::info("I2C Manager task stopped");
}

TaskHandle_t I2CManager::getTaskHandle() {
    return taskHandle;
}

bool I2CManager::queueRequest(const I2CRequest& request) {
    QueueHandle_t queue = FreeRTOSManager::getI2CRequestQueue();
    if (!queue) return false;

    BaseType_t result = xQueueSend(queue, &request, pdMS_TO_TICKS(100));
    return (result == pdTRUE);
}

bool I2CManager::executeRequest(const I2CRequest& request) {
    if (!initialized) return false;

    bool success = false;
    transactionCount++;

    if (request.writeLength > 0 && request.readLength > 0) {
        // Write-then-read operation
        success = performWriteRead(request.deviceAddress, request.writeData, request.writeLength,
                                 request.readBuffer, request.readLength);
    } else if (request.writeLength > 0) {
        // Write-only operation
        success = performWrite(request.deviceAddress, request.writeData, request.writeLength);
    } else if (request.readLength > 0) {
        // Read-only operation
        success = performRead(request.deviceAddress, request.readBuffer, request.readLength);
    }

    if (!success) {
        errorCount++;
        handleI2CError(request.deviceAddress, "transaction");
    }

    // Signal completion
    if (request.successFlag) {
        *request.successFlag = success;
    }

    if (request.completionSemaphore) {
        xSemaphoreGive(request.completionSemaphore);
    }

    return success;
}

// =============================================================================
// PRIVATE METHODS
// =============================================================================

void I2CManager::i2cManagerTask(void* parameter) {
    Logger::info("I2C Manager task started");

    QueueHandle_t requestQueue = FreeRTOSManager::getI2CRequestQueue();
    I2CRequest request;

    uint32_t lastHealthCheck = 0;
    uint32_t lastDeviceScan = 0;

    while (true) {
        // Process I2C requests
        if (xQueueReceive(requestQueue, &request, pdMS_TO_TICKS(100)) == pdTRUE) {
            executeRequest(request);
        }

        uint32_t now = millis();

        // Periodic health checks (every 5 seconds)
        if (now - lastHealthCheck >= 5000) {
            updateBusHealth();
            lastHealthCheck = now;
        }

        // Periodic device scanning (every 30 seconds)
        if (now - lastDeviceScan >= 30000) {
            scanForDevices();
            lastDeviceScan = now;
        }

        // Feed watchdog
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());
    }
}

bool I2CManager::initializeBus() {
    Logger::infof("Initializing I2C bus (SDA: %d, SCL: %d, Speed: %d Hz)",
                 I2C_SDA_PIN, I2C_SCL_PIN, I2C_CLOCK_SPEED);

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_CLOCK_SPEED);

    // Test bus functionality
    Wire.beginTransmission(0x00);  // General call address
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
        Logger::warning("Unexpected response from general call - bus may have issues");
    }

    Logger::info("I2C bus initialized successfully");
    return true;
}

bool I2CManager::performWrite(uint8_t deviceAddress, uint8_t* data, size_t length) {
    if (!data || length == 0) return false;

    Wire.beginTransmission(deviceAddress);
    size_t written = Wire.write(data, length);
    uint8_t error = Wire.endTransmission();

    bool success = (error == 0 && written == length);
    logI2COperation(deviceAddress, "write", success);

    return success;
}

bool I2CManager::performRead(uint8_t deviceAddress, uint8_t* buffer, size_t length) {
    if (!buffer || length == 0) return false;

    size_t received = Wire.requestFrom(deviceAddress, length);
    if (received != length) {
        logI2COperation(deviceAddress, "read", false);
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        if (Wire.available()) {
            buffer[i] = Wire.read();
        } else {
            logI2COperation(deviceAddress, "read", false);
            return false;
        }
    }

    logI2COperation(deviceAddress, "read", true);
    return true;
}

bool I2CManager::performWriteRead(uint8_t deviceAddress, uint8_t* writeData, size_t writeLength,
                                uint8_t* readBuffer, size_t readLength) {
    // First perform write
    if (!performWrite(deviceAddress, writeData, writeLength)) {
        return false;
    }

    // Small delay between write and read
    vTaskDelay(pdMS_TO_TICKS(1));

    // Then perform read
    return performRead(deviceAddress, readBuffer, readLength);
}

void I2CManager::scanForDevices() {
    uint8_t newDeviceCount = 0;
    memset(connectedDevices, 0, sizeof(connectedDevices));

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            // Device found
            connectedDevices[address / 8] |= (1 << (address % 8));
            newDeviceCount++;
        }

        vTaskDelay(pdMS_TO_TICKS(1));  // Small delay between scans
    }

    if (newDeviceCount != connectedDeviceCount) {
        Logger::infof("Device count changed: %d -> %d", connectedDeviceCount, newDeviceCount);
        connectedDeviceCount = newDeviceCount;
    }

    lastScanTime = millis();
}

void I2CManager::updateBusHealth() {
    bool healthy = isBusHealthy();

    if (!healthy && shouldAttemptRecovery()) {
        Logger::warning("Attempting I2C bus recovery...");
        recoverBus();
    }
}

bool I2CManager::recoverBus() {
    if (busRecoveryInProgress) return false;

    busRecoveryInProgress = true;
    lastRecoveryTime = millis();
    recoveryAttempts++;

    Logger::infof("I2C bus recovery attempt #%d", recoveryAttempts);

    // Reset I2C bus
    Wire.end();
    vTaskDelay(pdMS_TO_TICKS(100));

    // Reinitialize
    bool success = initializeBus();

    if (success) {
        Logger::info("I2C bus recovery successful");
        recoveryAttempts = 0;
    } else {
        Logger::error("I2C bus recovery failed");
    }

    busRecoveryInProgress = false;
    return success;
}

bool I2CManager::shouldAttemptRecovery() {
    // Don't attempt recovery too frequently
    if (millis() - lastRecoveryTime < 10000) return false;  // 10 second minimum

    // Don't attempt too many recoveries
    if (recoveryAttempts >= 3) return false;

    // Only attempt if success rate is very poor
    return getSuccessRate() < 0.5f;
}

void I2CManager::handleI2CError(uint8_t deviceAddress, const char* operation) {
    Logger::warningf("I2C error: device 0x%02X, operation: %s", deviceAddress, operation);

    // Report system alert for critical errors
    if (getSuccessRate() < 0.8f) {
        FreeRTOSManager::reportSystemAlert(2, 0x1001, "I2C communication errors");
    }
}

void I2CManager::logI2COperation(uint8_t deviceAddress, const char* operation, bool success) {
    Logger::debugf("I2C %s to 0x%02X: %s", operation, deviceAddress, success ? "OK" : "FAIL");
}

uint32_t I2CManager::calculateTimeout(size_t dataLength) {
    // Base timeout + time for data transfer
    return I2C_TIMEOUT_MS + (dataLength * 10);  // ~10ms per byte
}

// =============================================================================
// I2C DEVICE BASE CLASS IMPLEMENTATION
// =============================================================================

I2CDevice::I2CDevice(uint8_t address, const char* name)
    : deviceAddress(address), lastCommunicationTime(0), communicationCount(0),
      errorCount(0), healthy(false) {

    strncpy(deviceName, name, sizeof(deviceName) - 1);
    deviceName[sizeof(deviceName) - 1] = '\0';
}

bool I2CDevice::isConnected() {
    return I2CManager::isDevicePresent(deviceAddress);
}

uint8_t I2CDevice::getAddress() const {
    return deviceAddress;
}

const char* I2CDevice::getName() const {
    return deviceName;
}

uint32_t I2CDevice::getLastCommunicationTime() const {
    return lastCommunicationTime;
}

bool I2CDevice::writeRegister(uint8_t registerAddress, uint8_t value) {
    bool success = I2CManager::writeRegister(deviceAddress, registerAddress, value);
    recordCommunication(success);
    return success;
}

bool I2CDevice::writeRegister16(uint8_t registerAddress, uint16_t value) {
    bool success = I2CManager::writeRegister16(deviceAddress, registerAddress, value);
    recordCommunication(success);
    return success;
}

bool I2CDevice::readRegister(uint8_t registerAddress, uint8_t* value) {
    bool success = I2CManager::readRegister(deviceAddress, registerAddress, value);
    recordCommunication(success);
    return success;
}

bool I2CDevice::readRegister16(uint8_t registerAddress, uint16_t* value) {
    bool success = I2CManager::readRegister16(deviceAddress, registerAddress, value);
    recordCommunication(success);
    return success;
}

bool I2CDevice::readData(uint8_t* buffer, size_t length) {
    bool success = I2CManager::readData(deviceAddress, buffer, length);
    recordCommunication(success);
    return success;
}

bool I2CDevice::writeData(uint8_t* data, size_t length) {
    bool success = I2CManager::writeData(deviceAddress, data, length);
    recordCommunication(success);
    return success;
}

void I2CDevice::updateHealth() {
    // Update health based on recent communication success
    float successRate = getSuccessRate();
    healthy = (successRate > 0.8f) && isConnected();

    if (!healthy) {
        Logger::warningf("Device %s (0x%02X) health poor: %.1f%% success rate",
                        deviceName, deviceAddress, successRate * 100);
    }
}

bool I2CDevice::isHealthy() const {
    return healthy;
}

float I2CDevice::getSuccessRate() const {
    if (communicationCount == 0) return 1.0f;
    return (float)(communicationCount - errorCount) / communicationCount;
}

void I2CDevice::recordCommunication(bool success) {
    communicationCount++;
    if (!success) {
        errorCount++;
    }
    lastCommunicationTime = millis();
}
