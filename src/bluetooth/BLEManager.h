#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLECharacteristic.h>

enum class BLEStatus {
    UNINITIALIZED,
    INITIALIZING,
    ADVERTISING,
    CONNECTED,
    DISCONNECTED,
    ERROR
};

class BLEManager {
public:
    // Initialization and lifecycle
    void initialize();
    void update();
    void shutdown();
    
    // Connection management
    bool isConnected();
    BLEStatus getStatus();
    void startAdvertising();
    void stopAdvertising();
    
    // Command handling
    bool hasNewCommand();
    String getLastCommand();
    void clearCommand();
    
    // Device information
    String getDeviceName();
    String getDeviceAddress();
    int getConnectedDeviceCount();
    
    // Connection callbacks
    void setConnectionCallback(void (*callback)(bool connected));
    void setCommandCallback(void (*callback)(const String& command));
    
    // Statistics
    unsigned long getConnectionTime();
    int getConnectionCount();
    int getCommandCount();
    unsigned long getLastCommandTime();

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCharacteristic;
    BLEAdvertising* pAdvertising;
    
    BLEStatus currentStatus;
    bool initialized;
    bool deviceConnected;
    bool oldDeviceConnected;
    String lastReceivedCommand;
    bool newCommandAvailable;
    unsigned long connectionStartTime;
    int connectionCount;
    int commandCount;
    unsigned long lastCommandTime;
    
    void (*connectionCallback)(bool connected);
    void (*commandCallback)(const String& command);
    
    // Callback classes
    class ServerCallbacks;
    class CharacteristicCallbacks;
    
    // Helper methods
    void handleConnectionEvents();
    void processIncomingCommand(const String& command);
    void logConnectionStatus();
    void notifyConnectionChange(bool connected);
    
    // Configuration
    static const uint32_t ADVERTISING_INTERVAL = 100;  // milliseconds
    static const uint16_t CONNECTION_INTERVAL_MIN = 6;   // 7.5ms
    static const uint16_t CONNECTION_INTERVAL_MAX = 12;  // 15ms
};

// Callback class declarations
class BLEManager::ServerCallbacks : public BLEServerCallbacks {
public:
    ServerCallbacks(BLEManager* manager) : bleManager(manager) {}
    
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    
private:
    BLEManager* bleManager;
};

class BLEManager::CharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(BLEManager* manager) : bleManager(manager) {}
    
    void onWrite(BLECharacteristic* pCharacteristic) override;
    
private:
    BLEManager* bleManager;
};

#endif
