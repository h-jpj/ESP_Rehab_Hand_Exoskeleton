#include "BLEManager.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"

void BLEManager::initialize() {
    if (initialized) return;

    Logger::info("Initializing BLE Manager...");

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

    try {
        // Initialize NimBLE
        NimBLEDevice::init(BLE_DEVICE_NAME);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Maximum power

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

        pCharacteristic->setCallbacks(new CharacteristicCallbacks(this));
        pCharacteristic->setValue("Ready");

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

        Logger::info("BLE Manager initialized successfully");

    } catch (const std::exception& e) {
        Logger::errorf("BLE initialization failed: %s", e.what());
        currentStatus = BLEStatus::ERROR;
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
    Logger::infof("BLE command received: %s", command.c_str());

    lastReceivedCommand = command;
    newCommandAvailable = true;
    commandCount++;
    lastCommandTime = millis();

    // Notify via callback if set
    if (commandCallback) {
        commandCallback(command);
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

// ServerCallbacks implementation
void BLEManager::ServerCallbacks::onConnect(BLEServer* pServer) {
    bleManager->deviceConnected = true;

    // Set connection parameters for better performance
    pServer->updateConnParams(0,  // Connection ID 0 for first connection
                             BLEManager::CONNECTION_INTERVAL_MIN,
                             BLEManager::CONNECTION_INTERVAL_MAX,
                             0, 400);
}

void BLEManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    bleManager->deviceConnected = false;
}

// CharacteristicCallbacks implementation
void BLEManager::CharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();

    if (value.length() > 0) {
        bleManager->processIncomingCommand(value);
    }
}
