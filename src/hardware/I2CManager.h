#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "FreeRTOSManager.h"

// =============================================================================
// I2C MANAGER CLASS
// =============================================================================

class I2CManager {
public:
    // Lifecycle
    static bool initialize();
    static void shutdown();
    static bool isInitialized();

    // I2C Operations (Thread-Safe)
    static bool writeRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t value);
    static bool writeRegister16(uint8_t deviceAddress, uint8_t registerAddress, uint16_t value);
    static bool writeData(uint8_t deviceAddress, uint8_t* data, size_t length);
    
    static bool readRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t* value);
    static bool readRegister16(uint8_t deviceAddress, uint8_t registerAddress, uint16_t* value);
    static bool readData(uint8_t deviceAddress, uint8_t* buffer, size_t length);
    static bool readRegisterData(uint8_t deviceAddress, uint8_t registerAddress, uint8_t* buffer, size_t length);

    // Device Management
    static bool scanDevices();
    static bool isDevicePresent(uint8_t deviceAddress);
    static void logConnectedDevices();

    // Bus Health
    static bool isBusHealthy();
    static uint32_t getTransactionCount();
    static uint32_t getErrorCount();
    static float getSuccessRate();

    // Task Management
    static void startTask();
    static void stopTask();
    static TaskHandle_t getTaskHandle();

    // Request Queue Interface
    static bool queueRequest(const I2CRequest& request);
    static bool executeRequest(const I2CRequest& request);

private:
    // Initialization state
    static bool initialized;
    static bool taskRunning;
    static TaskHandle_t taskHandle;

    // Bus statistics
    static uint32_t transactionCount;
    static uint32_t errorCount;
    static uint32_t lastScanTime;
    static uint8_t connectedDevices[128];  // Bit array for device presence
    static uint8_t connectedDeviceCount;

    // Bus recovery
    static bool busRecoveryInProgress;
    static uint32_t lastRecoveryTime;
    static uint8_t recoveryAttempts;

    // Task function
    static void i2cManagerTask(void* parameter);

    // Internal I2C operations (not thread-safe, use within task only)
    static bool performWrite(uint8_t deviceAddress, uint8_t* data, size_t length);
    static bool performRead(uint8_t deviceAddress, uint8_t* buffer, size_t length);
    static bool performWriteRead(uint8_t deviceAddress, uint8_t* writeData, size_t writeLength, 
                                uint8_t* readBuffer, size_t readLength);

    // Bus management
    static bool initializeBus();
    static bool recoverBus();
    static void updateBusHealth();
    static void scanForDevices();

    // Error handling
    static void handleI2CError(uint8_t deviceAddress, const char* operation);
    static bool shouldAttemptRecovery();

    // Utility methods
    static void logI2COperation(uint8_t deviceAddress, const char* operation, bool success);
    static uint32_t calculateTimeout(size_t dataLength);
};

// =============================================================================
// I2C DEVICE BASE CLASS
// =============================================================================

class I2CDevice {
public:
    I2CDevice(uint8_t address, const char* name);
    virtual ~I2CDevice() = default;

    // Device lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isConnected();

    // Device information
    uint8_t getAddress() const;
    const char* getName() const;
    uint32_t getLastCommunicationTime() const;

    // Communication helpers
    bool writeRegister(uint8_t registerAddress, uint8_t value);
    bool writeRegister16(uint8_t registerAddress, uint16_t value);
    bool readRegister(uint8_t registerAddress, uint8_t* value);
    bool readRegister16(uint8_t registerAddress, uint16_t* value);
    bool readData(uint8_t* buffer, size_t length);
    bool writeData(uint8_t* data, size_t length);

    // Health monitoring
    virtual bool performSelfTest() = 0;
    virtual void updateHealth();
    bool isHealthy() const;
    float getSuccessRate() const;

protected:
    uint8_t deviceAddress;
    char deviceName[32];
    uint32_t lastCommunicationTime;
    uint32_t communicationCount;
    uint32_t errorCount;
    bool healthy;

    // Update communication statistics
    void recordCommunication(bool success);
};

// =============================================================================
// SENSOR DATA STRUCTURES
// =============================================================================

// Base sensor reading structure
struct SensorReading {
    uint32_t timestamp;
    float quality;
    bool valid;
    uint8_t sensorId;
};

// Pulse sensor data
struct PulseReading : public SensorReading {
    uint16_t heartRate;      // BPM
    uint8_t spO2;           // Oxygen saturation %
    uint32_t redValue;      // Raw red LED value
    uint32_t irValue;       // Raw IR LED value
    float signalStrength;   // Signal quality indicator
};

// Motion sensor data
struct MotionReading : public SensorReading {
    float accelX, accelY, accelZ;    // m/s²
    float gyroX, gyroY, gyroZ;       // °/s
    float temperature;               // °C
    bool motionDetected;
    float movementIntensity;
};

// Pressure sensor data
struct PressureReading : public SensorReading {
    float force;            // Newtons
    float pressure;         // kPa
    uint16_t rawValue;      // ADC reading
    uint8_t sensorIndex;    // Which pressure sensor (0-3)
};

// Fused sensor data
struct FusedSensorData {
    uint32_t timestamp;
    PulseReading pulse;
    MotionReading motion;
    PressureReading pressure[4];  // Support up to 4 pressure sensors
    float overallQuality;
    bool allSensorsValid;
    uint8_t activeSensorCount;
};

#endif // I2C_MANAGER_H
