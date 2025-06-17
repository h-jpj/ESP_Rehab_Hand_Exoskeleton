#ifndef PULSE_MONITOR_MANAGER_H
#define PULSE_MONITOR_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "../config/Config.h"

// Forward declarations
class MAX30105;

enum class PulseQuality {
    EXCELLENT,
    GOOD,
    FAIR,
    POOR,
    NO_SIGNAL
};

struct HeartRateReading {
    unsigned long timestamp;
    float heartRate;
    float spO2;
    uint32_t irValue;
    uint32_t redValue;
    PulseQuality quality;
    bool fingerDetected;
    float signalStrength;
};

struct PulseMetrics {
    float averageHeartRate;
    float minHeartRate;
    float maxHeartRate;
    float averageSpO2;
    float minSpO2;
    float maxSpO2;
    unsigned long sessionDuration;
    int totalReadings;
    int validReadings;
    float dataQuality;
    PulseQuality overallQuality;
};

struct PulseAlert {
    unsigned long timestamp;
    String alertType;
    String message;
    float value;
    bool critical;
};

class PulseMonitorManager {
public:
    // Initialization and lifecycle
    void initialize();
    void shutdown();
    
    // FreeRTOS task management
    void startTask();
    void stopTask();
    bool isTaskRunning();
    
    // Sensor operations
    bool isSensorConnected();
    bool isFingerDetected();
    HeartRateReading getCurrentReading();
    PulseMetrics getSessionMetrics();

    // Data access
    bool hasNewReading();
    HeartRateReading getLatestReading();
    void clearNewReading();
    
    // Configuration
    void setSamplingRate(uint16_t rate);
    void setPulseAmplitude(uint8_t amplitude);
    void setSampleAverage(uint8_t samples);
    void setLEDMode(uint8_t mode);
    
    // Calibration and quality
    void startCalibration();
    bool isCalibrated();
    PulseQuality assessSignalQuality();
    float getSignalStrength();
    
    // Session management
    void startSession();
    void endSession();
    bool isSessionActive();
    void resetMetrics();
    
    // Alerts and thresholds
    void setHeartRateThresholds(float min, float max);
    void setSpO2Thresholds(float min, float max);
    bool hasNewAlerts();
    PulseAlert getLatestAlert();
    void clearAlerts();
    
    // Statistics
    uint32_t getTotalReadings();
    uint32_t getValidReadings();
    float getDataQualityPercent();
    unsigned long getSessionDuration();
    
    // Callbacks
    void setReadingCallback(void (*callback)(const HeartRateReading& reading));
    void setAlertCallback(void (*callback)(const PulseAlert& alert));

private:
    bool initialized;
    bool taskRunning;
    bool sessionActive;
    bool calibrated;
    TaskHandle_t taskHandle;
    
    // Sensor instance (using global instance)
    // MAX30105* sensor; // Not needed, using global sensor
    
    // Current state
    HeartRateReading currentReading;
    PulseMetrics sessionMetrics;
    bool newReadingAvailable;
    unsigned long sessionStartTime;
    unsigned long lastReadingTime;
    
    // Configuration
    uint16_t samplingRate;
    uint8_t pulseAmplitude;
    uint8_t sampleAverage;
    uint8_t ledMode;
    
    // Thresholds and alerts
    float heartRateMin;
    float heartRateMax;
    float spO2Min;
    float spO2Max;
    PulseAlert latestAlert;
    bool newAlertAvailable;
    
    // Data processing
    struct ReadingBuffer {
        float heartRates[10];
        float spO2Values[10];
        uint32_t irValues[10];
        uint32_t redValues[10];
        int index;
        int count;
    };
    ReadingBuffer buffer;
    
    // Calibration
    unsigned long calibrationStartTime;
    bool calibrationInProgress;
    int calibrationReadings;
    float baselineIR;
    float baselineRed;
    
    // Task function
    static void pulseMonitorTask(void* parameter);
    
    // Callbacks
    void (*readingCallback)(const HeartRateReading& reading);
    void (*alertCallback)(const PulseAlert& alert);
    
    // Internal methods
    void updateSensor();
    void processReading();
    void calculateMetrics();
    void checkThresholds();
    void updateBuffer(const HeartRateReading& reading);
    
    // Signal processing
    float calculateHeartRate();
    float calculateSpO2();
    PulseQuality assessQuality(uint32_t irValue, uint32_t redValue);
    float calculateSignalStrength(uint32_t irValue, uint32_t redValue);
    bool detectFinger(uint32_t irValue);
    
    // Calibration methods
    void performCalibration();
    void updateCalibration(uint32_t irValue, uint32_t redValue);
    bool isCalibrationComplete();
    
    // Alert generation
    void generateAlert(const String& type, const String& message, float value, bool critical = false);
    void checkHeartRateAlert(float heartRate);
    void checkSpO2Alert(float spO2);
    void checkSignalQualityAlert(PulseQuality quality);
    
    // Utility methods
    float averageArray(float* array, int count);
    void resetBuffer();
    void updateSessionMetrics();

    // I2C communication helpers
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    bool readFIFO(uint32_t* redValue, uint32_t* irValue);
    
    // Configuration constants
    static const uint16_t DEFAULT_SAMPLING_RATE = 100;  // 100Hz
    static const uint8_t DEFAULT_PULSE_AMPLITUDE = 0x1F;  // Medium amplitude
    static const uint8_t DEFAULT_SAMPLE_AVERAGE = 4;  // 4 samples average
    static const uint8_t DEFAULT_LED_MODE = 2;  // Red + IR mode
    static const unsigned long READING_TIMEOUT = 1000;  // 1 second
    static const unsigned long CALIBRATION_TIME = 10000;  // 10 seconds
    static const int MIN_CALIBRATION_READINGS = 50;
    static constexpr float MIN_SIGNAL_THRESHOLD = 50000.0f;
    static constexpr float DEFAULT_HR_MIN = 50.0f;
    static constexpr float DEFAULT_HR_MAX = 150.0f;
    static constexpr float DEFAULT_SPO2_MIN = 90.0f;
    static constexpr float DEFAULT_SPO2_MAX = 100.0f;
};

#endif
