#include "BLEManager.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "../hardware/FreeRTOSManager.h"
#include "../memory/StaticBLEMemory.h"
#include "../memory/NimBLEStaticConfig.h"

// Base64 decoding functions
extern "C" {
    unsigned char* base64_decode(const unsigned char* src, size_t len, size_t* out_len);
}

void BLEManager::initialize() {
    if (initialized) return;

    Logger::info("Initializing BLE Manager with Static Memory...");

    currentStatus = BLEStatus::INITIALIZING;
    deviceConnected = false;
    oldDeviceConnected = false;
    newCommandAvailable = false;
    connectionStartTime = 0;
    connectionCount = 0;
    commandCount = 0;
    lastCommandTime = 0;
    connectionCallback = nullptr;
    commandCallback = nullptr;
    taskHandle = nullptr;
    taskRunning = false;

    try {
        // CRITICAL: Initialize static BLE memory BEFORE NimBLE
        Logger::info("Initializing Static BLE Memory Pool...");
        if (!StaticBLEMemory::initialize()) {
            Logger::error("Failed to initialize Static BLE Memory");
            currentStatus = BLEStatus::ERROR;
            REPORT_ERROR(ErrorCode::BLE_INITIALIZATION_FAILED, "Static BLE Memory initialization failed");
            return;
        }

        // Configure NimBLE to use static memory
        Logger::info("Configuring NimBLE for Static Memory...");
        if (!NimBLEStaticConfig::configure()) {
            Logger::error("Failed to configure NimBLE for static memory");
            currentStatus = BLEStatus::ERROR;
            REPORT_ERROR(ErrorCode::BLE_INITIALIZATION_FAILED, "NimBLE static configuration failed");
            return;
        }

        // Log memory status before NimBLE initialization
        StaticBLEMemory::logMemoryStatus();

        // Initialize NimBLE with static memory configuration
        Logger::info("Initializing NimBLE with static memory...");
        NimBLEDevice::init(BLE_DEVICE_NAME);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Maximum power

        // Log memory usage after NimBLE initialization
        Logger::info("NimBLE initialized, checking memory usage...");
        StaticBLEMemory::logMemoryStatus();

        // Create BLE Server
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // Create BLE Service
        pService = pServer->createService(BLE_SERVICE_UUID);

        // Create BLE Characteristic
        pCharacteristic = pService->createCharacteristic(
            BLE_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );

        Logger::infof("BLE Characteristic created with UUID: %s", BLE_CHARACTERISTIC_UUID);
        Logger::infof("BLE Characteristic properties: READ | WRITE | NOTIFY");

        pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
        pCharacteristic->setValue("Ready");

        Logger::info("BLE Characteristic callbacks set and initial value set to 'Ready'");

        // Start the service
        pService->start();

        // Get advertising object and configure it
        pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
        pAdvertising->setScanResponse(false);
        pAdvertising->setMinPreferred(0x0);  // Set value to 0x00 to not advertise this parameter

        initialized = true;
        currentStatus = BLEStatus::DISCONNECTED;

        startAdvertising();

        // Start the BLE management task
        startTask();

        Logger::info("BLE Manager initialized successfully with Static Memory and FreeRTOS task");

        // Final memory status check
        if (!StaticBLEMemory::isHealthy()) {
            Logger::warning("BLE Static Memory health check failed after initialization");
        }

        // Log final memory statistics
        BLEMemoryStats stats = getBLEMemoryStats();
        Logger::infof("BLE Memory: %d/%d bytes used (%.1f%%), %lu allocations",
                     stats.usedSize, stats.totalSize,
                     (float)stats.usedSize / stats.totalSize * 100.0f,
                     stats.allocationCount);

    } catch (const std::exception& e) {
        Logger::errorf("BLE initialization failed: %s", e.what());
        currentStatus = BLEStatus::ERROR;

        // Clean up static memory on failure
        StaticBLEMemory::shutdown();
        NimBLEStaticConfig::restore();

        REPORT_ERROR(ErrorCode::BLE_INITIALIZATION_FAILED, "BLE initialization exception");
    }
}

void BLEManager::update() {
    if (!initialized || currentStatus == BLEStatus::ERROR) return;

    handleConnectionEvents();

    // Handle advertising restart if needed
    if (!deviceConnected && currentStatus != BLEStatus::ADVERTISING) {
        startAdvertising();
    }
}

void BLEManager::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down BLE Manager...");

    stopTask();
    stopAdvertising();

    if (pServer) {
        pServer->disconnect(0);  // Disconnect all connections
    }

    NimBLEDevice::deinit(true);

    initialized = false;
    currentStatus = BLEStatus::UNINITIALIZED;

    Logger::info("BLE Manager shutdown complete");
}

bool BLEManager::isConnected() {
    return deviceConnected && currentStatus == BLEStatus::CONNECTED;
}

BLEStatus BLEManager::getStatus() {
    return currentStatus;
}

void BLEManager::startAdvertising() {
    if (!initialized || currentStatus == BLEStatus::ERROR) return;

    try {
        Logger::info("Starting BLE advertising...");

        // Configure advertising parameters
        pAdvertising->setAdvertisementType(BLE_GAP_CONN_MODE_UND);
        pAdvertising->setMinInterval(ADVERTISING_INTERVAL);
        pAdvertising->setMaxInterval(ADVERTISING_INTERVAL);

        NimBLEDevice::startAdvertising();

        currentStatus = BLEStatus::ADVERTISING;
        Logger::infof("BLE advertising started with improved parameters (Device: %s)", BLE_DEVICE_NAME);

    } catch (const std::exception& e) {
        Logger::errorf("Failed to start BLE advertising: %s", e.what());
        currentStatus = BLEStatus::ERROR;
        REPORT_ERROR(ErrorCode::BLE_INITIALIZATION_FAILED, "BLE advertising failed");
    }
}

void BLEManager::stopAdvertising() {
    if (!initialized) return;

    try {
        Logger::info("Stopping BLE advertising...");
        NimBLEDevice::stopAdvertising();

        if (currentStatus == BLEStatus::ADVERTISING) {
            currentStatus = BLEStatus::DISCONNECTED;
        }

    } catch (const std::exception& e) {
        Logger::errorf("Failed to stop BLE advertising: %s", e.what());
    }
}

bool BLEManager::hasNewCommand() {
    return newCommandAvailable;
}

String BLEManager::getLastCommand() {
    return lastReceivedCommand;
}

void BLEManager::clearCommand() {
    newCommandAvailable = false;
    lastReceivedCommand = "";
}

String BLEManager::getDeviceName() {
    return String(BLE_DEVICE_NAME);
}

String BLEManager::getDeviceAddress() {
    if (initialized) {
        return NimBLEDevice::getAddress().toString().c_str();
    }
    return "";
}

int BLEManager::getConnectedDeviceCount() {
    if (pServer && deviceConnected) {
        return pServer->getConnectedCount();
    }
    return 0;
}

void BLEManager::setConnectionCallback(void (*callback)(bool connected)) {
    connectionCallback = callback;
}

void BLEManager::setCommandCallback(void (*callback)(const String& command)) {
    commandCallback = callback;
}

unsigned long BLEManager::getConnectionTime() {
    if (isConnected() && connectionStartTime > 0) {
        return millis() - connectionStartTime;
    }
    return 0;
}

int BLEManager::getConnectionCount() {
    return connectionCount;
}

int BLEManager::getCommandCount() {
    return commandCount;
}

unsigned long BLEManager::getLastCommandTime() {
    return lastCommandTime;
}

void BLEManager::handleConnectionEvents() {
    // Handle connection state changes
    if (deviceConnected != oldDeviceConnected) {
        if (deviceConnected) {
            Logger::info("BLE device connected");
            currentStatus = BLEStatus::CONNECTED;
            connectionStartTime = millis();
            connectionCount++;
            notifyConnectionChange(true);
        } else {
            Logger::info("BLE device disconnected");
            currentStatus = BLEStatus::DISCONNECTED;
            notifyConnectionChange(false);

            // Restart advertising after a brief delay
            delay(500);
            startAdvertising();
        }

        oldDeviceConnected = deviceConnected;
    }
}

void BLEManager::processIncomingCommand(const String& command) {
    Logger::infof("BLE command received: '%s' (length: %d)", command.c_str(), command.length());

    lastReceivedCommand = command;
    newCommandAvailable = true;
    commandCount++;
    lastCommandTime = millis();

    // Notify via callback if set
    if (commandCallback) {
        Logger::info("BLE calling command callback...");
        commandCallback(command);
        Logger::info("BLE command callback completed");
    } else {
        Logger::warning("BLE command callback is not set!");
    }
}

void BLEManager::logConnectionStatus() {
    switch (currentStatus) {
        case BLEStatus::UNINITIALIZED:
            Logger::info("BLE Status: Uninitialized");
            break;
        case BLEStatus::INITIALIZING:
            Logger::info("BLE Status: Initializing...");
            break;
        case BLEStatus::ADVERTISING:
            Logger::info("BLE Status: Advertising");
            break;
        case BLEStatus::CONNECTED:
            Logger::info("BLE Status: Connected");
            break;
        case BLEStatus::DISCONNECTED:
            Logger::info("BLE Status: Disconnected");
            break;
        case BLEStatus::ERROR:
            Logger::error("BLE Status: Error");
            break;
    }
}

void BLEManager::notifyConnectionChange(bool connected) {
    if (connectionCallback) {
        connectionCallback(connected);
    }
}

void BLEManager::sendTestNotification(const String& message) {
    if (initialized && pCharacteristic && deviceConnected) {
        Logger::infof("BLE sending test notification: %s", message.c_str());
        pCharacteristic->setValue(message.c_str());
        pCharacteristic->notify();
    } else {
        Logger::warning("BLE cannot send notification - not connected or not initialized");
    }
}

void BLEManager::simulateCommand(const String& command) {
    Logger::infof("BLE simulating command: %s", command.c_str());
    processIncomingCommand(command);
}

String BLEManager::decodeBase64Command(const String& encodedCommand) {
    // Handle empty input
    if (encodedCommand.length() == 0) {
        Logger::debug("BLE Base64 decode: empty input");
        return "";
    }

    // Quick check: if it's a simple command (single digit or known command), it's probably plain text
    if (encodedCommand.length() <= 2 && (encodedCommand == "0" || encodedCommand == "1" || encodedCommand == "2")) {
        Logger::debugf("BLE Base64 decode: '%s' appears to be plain text, skipping decode", encodedCommand.c_str());
        return "";  // Return empty to indicate no Base64 decoding needed
    }

    try {
        // Use ESP32 base64 library to decode
        // The densaugeo/base64 library uses decode() as a static function
        unsigned char* decoded_data = nullptr;
        size_t decoded_length = 0;

        // Decode the base64 string
        decoded_data = base64_decode((const unsigned char*)encodedCommand.c_str(),
                                   encodedCommand.length(), &decoded_length);

        if (decoded_data == nullptr || decoded_length == 0) {
            Logger::debugf("BLE Base64 decode: failed for '%s' (probably plain text)", encodedCommand.c_str());
            if (decoded_data) free(decoded_data);
            return "";
        }

        // Convert to String and ensure null termination
        String decoded = "";
        for (size_t i = 0; i < decoded_length; i++) {
            decoded += (char)decoded_data[i];
        }

        // Clean up allocated memory
        free(decoded_data);

        // Trim any whitespace
        decoded.trim();

        // Validate that the decoded result makes sense
        if (decoded.length() == 0 || decoded.length() > 20) {
            Logger::debugf("BLE Base64 decode: invalid result length for '%s'", encodedCommand.c_str());
            return "";
        }

        Logger::debugf("BLE Base64 decode successful: '%s' -> '%s'",
                      encodedCommand.c_str(), decoded.c_str());

        return decoded;

    } catch (const std::exception& e) {
        Logger::debugf("BLE Base64 decode exception: %s (probably plain text)", e.what());
        return "";
    } catch (...) {
        Logger::debugf("BLE Base64 decode unknown exception for: '%s' (probably plain text)", encodedCommand.c_str());
        return "";
    }
}

// ServerCallbacks implementation
void BLEManager::ServerCallbacks::onConnect(BLEServer* pServer) {
    Logger::info("BLE ServerCallbacks::onConnect called");
    bleManager->deviceConnected = true;

    // Set connection parameters for better performance
    pServer->updateConnParams(0,  // Connection ID 0 for first connection
                             BLEManager::CONNECTION_INTERVAL_MIN,
                             BLEManager::CONNECTION_INTERVAL_MAX,
                             0, 400);

    Logger::info("BLE connection parameters updated");
}

void BLEManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    Logger::info("BLE ServerCallbacks::onDisconnect called");
    bleManager->deviceConnected = false;
}

// CharacteristicCallbacks implementation
void BLEManager::CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string stdValue = pCharacteristic->getValue();
    String rawValue = stdValue.c_str();

    Logger::infof("BLE onWrite called - Raw data length: %d", stdValue.length());
    Logger::infof("BLE onWrite called - Raw value: '%s'", rawValue.c_str());

    if (rawValue.length() > 0) {
        String commandToProcess = rawValue;

        // Try Base64 decoding first, fall back to plain text if it fails
        String decodedCommand = bleManager->decodeBase64Command(rawValue);

        if (decodedCommand.length() > 0) {
            // Base64 decoding successful
            Logger::infof("BLE decoded Base64 command: '%s' (from: '%s')",
                         decodedCommand.c_str(), rawValue.c_str());
            commandToProcess = decodedCommand;
        } else {
            // Base64 decoding failed, assume plain text
            Logger::infof("BLE using plain text command: '%s'", rawValue.c_str());
            commandToProcess = rawValue;
        }

        Logger::infof("BLE processing command: '%s'", commandToProcess.c_str());
        bleManager->processIncomingCommand(commandToProcess);
    } else {
        Logger::warning("BLE onWrite called but value is empty");
    }
}

// =============================================================================
// FREERTOS TASK MANAGEMENT
// =============================================================================

void BLEManager::startTask() {
    if (taskRunning || taskHandle) return;

    BaseType_t result = xTaskCreatePinnedToCore(
        bleServerTask,
        "BLEServer",
        TASK_STACK_BLE_SERVER,
        this,
        PRIORITY_BLE_SERVER,
        &taskHandle,
        CORE_PROTOCOL  // Core 0 for protocol tasks
    );

    if (result == pdPASS) {
        taskRunning = true;
        Logger::info("BLE Server task started on Core 0");
    } else {
        Logger::error("Failed to create BLE Server task");
    }
}

void BLEManager::stopTask() {
    if (!taskRunning || !taskHandle) return;

    taskRunning = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;

    Logger::info("BLE Server task stopped");
}

bool BLEManager::isTaskRunning() {
    return taskRunning && taskHandle != nullptr;
}

void BLEManager::bleServerTask(void* parameter) {
    BLEManager* manager = (BLEManager*)parameter;
    Logger::info("BLE Server task started");

    while (manager->taskRunning) {
        // Handle connection events and status updates
        manager->handleConnectionEvents();

        // Handle advertising restart if needed
        if (!manager->deviceConnected && manager->currentStatus != BLEStatus::ADVERTISING) {
            manager->startAdvertising();
        }

        // Feed watchdog - now that FreeRTOS Manager is re-enabled
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());

        // Task runs every 100ms for connection management
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    Logger::info("BLE Server task ended");
    vTaskDelete(nullptr);  // Delete self
}
