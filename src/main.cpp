#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

// Hardware Configuration
#define SERVO_PIN_1 19
#define SERVO_PIN_2 22
#define SERVO_PIN_3 23

// BLE Configuration
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DEVICE_NAME         "ESP32_Servo"

// Global Objects
Servo servo1, servo2, servo3;
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// WiFi and MQTT Objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool wifiConnected = false;
bool mqttConnected = false;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastStatusReport = 0;

// Task Management
TaskHandle_t servoTaskHandle = NULL;
volatile uint8_t currentState = 0;
volatile bool interruptSequence = false;
volatile bool stateChanged = false;

// Servo positions
const int SERVO_MIN = 0;
const int SERVO_MAX = 90;
const int MOVEMENT_DELAY = 1000;
const int STAGGER_DELAY = 100;

// Forward declarations
void publishMovementCommand(const String& command, unsigned long responseTime);
void publishWiFiStatus(const String& status);
void publishBLEStatus(const String& status);
void publishSystemStatus();

// Time synchronization
void initializeTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for NTP time sync...");

    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 8 * 3600 * 2 && attempts < 20) {  // Wait for valid time
        delay(500);
        now = time(nullptr);
        attempts++;
    }

    if (now > 8 * 3600 * 2) {
        Serial.printf("Time synchronized: %s", ctime(&now));
    } else {
        Serial.println("Failed to sync time, using millis() fallback");
    }
}

// Get current Unix timestamp
unsigned long getCurrentTimestamp() {
    time_t now = time(nullptr);
    if (now > 1640995200) {  // After Jan 1, 2022 (valid timestamp)
        Serial.printf("Using NTP time: %lu\n", now);
        return now;
    } else {
        // Fallback: Use a fixed recent timestamp + uptime
        // May 30, 2025 05:00:00 UTC = 1748494800
        unsigned long timestamp = 1748494800 + (millis() / 1000);
        Serial.printf("Using fallback time: %lu (millis: %lu)\n", timestamp, millis());
        return timestamp;
    }
}

// WiFi Management Functions
void scanWiFiNetworks() {
    Serial.println("Scanning for WiFi networks...");
    int n = WiFi.scanNetworks();
    Serial.printf("Found %d networks:\n", n);

    for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s (%d dBm) %s\n",
                     i + 1,
                     WiFi.SSID(i).c_str(),
                     WiFi.RSSI(i),
                     WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
    }
    Serial.println();
}

void initializeWiFi() {
    Serial.println("Initializing WiFi...");
    WiFi.mode(WIFI_STA);

    // Scan for networks first to help with debugging
    scanWiFiNetworks();

    Serial.printf("Attempting to connect to: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Don't block here - we'll check connection status in loop
    lastWifiAttempt = millis();
}

void checkWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnected) {
            wifiConnected = true;
            Serial.println("WiFi connected!");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());

            // Initialize time synchronization now that WiFi is connected
            initializeTime();

            // Publish WiFi connection event
            publishWiFiStatus("connected");
        }
    } else {
        if (wifiConnected) {
            wifiConnected = false;
            mqttConnected = false;
            Serial.println("WiFi disconnected!");
        }

        // Try to reconnect if enough time has passed
        if (millis() - lastWifiAttempt > WIFI_RECONNECT_INTERVAL) {
            Serial.println("Attempting WiFi reconnection...");
            WiFi.reconnect();
            lastWifiAttempt = millis();
        }
    }
}

// MQTT Management Functions
void initializeMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(512);  // Increase buffer size for larger messages
    Serial.printf("MQTT server configured: %s:%d (buffer: 512 bytes)\n", MQTT_SERVER, MQTT_PORT);
}

void checkMQTTConnection() {
    if (!wifiConnected) {
        return; // Can't connect to MQTT without WiFi
    }

    if (mqttClient.connected()) {
        if (!mqttConnected) {
            mqttConnected = true;
            Serial.println("MQTT connected!");
            publishSystemStatus();
        }

        // Process MQTT messages (non-blocking)
        mqttClient.loop();
    } else {
        if (mqttConnected) {
            mqttConnected = false;
            Serial.println("MQTT disconnected!");
        }

        // Try to reconnect if enough time has passed
        if (millis() - lastMqttAttempt > MQTT_RECONNECT_INTERVAL) {
            Serial.println("Attempting MQTT connection...");

            String clientId = String(DEVICE_ID) + "_" + String(random(0xffff), HEX);

            if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
                Serial.println("MQTT connection successful!");
                mqttConnected = true;

                // Test with a simple message first
                if (mqttClient.publish("rehab_exo/ESP32_001/test", "hello")) {
                    Serial.println("✅ Test message published successfully");
                } else {
                    Serial.println("❌ Test message failed");
                }

                publishSystemStatus();
            } else {
                Serial.printf("MQTT connection failed, rc=%d\n", mqttClient.state());
            }

            lastMqttAttempt = millis();
        }
    }
}

// MQTT Publishing Functions
void publishMovementCommand(const String& command, unsigned long responseTime) {
    if (!mqttConnected) return;

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = getCurrentTimestamp();
    doc["event_type"] = "movement_command";

    JsonObject data = doc["data"].to<JsonObject>();
    data["command"] = command;
    data["response_time_ms"] = responseTime;
    data["ble_connected"] = deviceConnected;

    String payload;
    serializeJson(doc, payload);

    if (mqttClient.publish(TOPIC_MOVEMENT_COMMAND, payload.c_str(), false)) {  // QoS 0, no retain
        Serial.printf("Published movement command: %s\n", command.c_str());
    } else {
        Serial.println("Failed to publish movement command");
    }
}

void publishWiFiStatus(const String& status) {
    if (!mqttConnected) return;

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = getCurrentTimestamp();
    doc["event_type"] = "wifi_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = status;
    if (status == "connected") {
        data["ip_address"] = WiFi.localIP().toString();
        data["rssi"] = WiFi.RSSI();
    }

    String payload;
    serializeJson(doc, payload);

    mqttClient.publish(TOPIC_CONNECTION_WIFI, payload.c_str());
}

void publishBLEStatus(const String& status) {
    if (!mqttConnected) return;

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = getCurrentTimestamp();
    doc["event_type"] = "ble_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = status;

    String payload;
    serializeJson(doc, payload);

    mqttClient.publish(TOPIC_CONNECTION_BLE, payload.c_str());
}

void publishSystemStatus() {
    if (!mqttConnected) {
        Serial.println("Cannot publish system status - MQTT not connected");
        return;
    }

    Serial.println("Publishing system status...");

    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["timestamp"] = getCurrentTimestamp();
    doc["event_type"] = "system_status";

    JsonObject data = doc["data"].to<JsonObject>();
    data["status"] = "online";
    data["firmware_version"] = FIRMWARE_VERSION;
    data["uptime_seconds"] = millis() / 1000;
    data["free_heap"] = ESP.getFreeHeap();
    data["wifi_connected"] = wifiConnected;
    data["ble_connected"] = deviceConnected;
    data["current_state"] = currentState;

    if (wifiConnected) {
        data["wifi_rssi"] = WiFi.RSSI();
        data["ip_address"] = WiFi.localIP().toString();
    }

    String payload;
    serializeJson(doc, payload);

    Serial.printf("Publishing to topic: %s\n", TOPIC_SYSTEM_STATUS);
    Serial.printf("Payload: %s\n", payload.c_str());

    if (mqttClient.publish(TOPIC_SYSTEM_STATUS, payload.c_str(), false)) {  // QoS 0, no retain
        Serial.println("✅ System status published successfully");
    } else {
        Serial.printf("❌ Failed to publish system status (state: %d, buffer: %d)\n",
                     mqttClient.state(), mqttClient.getBufferSize());
    }
}

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected - keeping connection stable");
        // Don't stop advertising immediately - let connection stabilize first
        delay(100);
        NimBLEDevice::getAdvertising()->stop();

        // Log BLE connection event
        publishBLEStatus("connected");
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        // Restart advertising after disconnect
        delay(1000); // Longer delay before restarting
        NimBLEDevice::startAdvertising();
        Serial.println("Advertising restarted after disconnect");

        // Log BLE disconnection event
        publishBLEStatus("disconnected");
    }
};

class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.printf("=== RECEIVED COMMAND: '%s' (length: %d) ===\n", value.c_str(), value.length());

            // Handle TEST command
            if (value == "TEST") {
                Serial.println("Running servo test...");
                // Run the test directly in this callback
                for (int i = 0; i < 3; i++) {
                    Serial.printf("Test cycle %d/3\n", i + 1);
                    servo3.write(90);
                    delay(1000);
                    servo3.write(0);
                    delay(1000);
                }
                Serial.println("Servo test complete!");

                // Send acknowledgment
                pCharacteristic->setValue("TEST_COMPLETE");
                pCharacteristic->notify();
                return;
            }

            // Handle RESTART_ADVERTISING command
            if (value == "RESTART_ADV") {
                Serial.println("Restarting BLE advertising...");
                NimBLEDevice::startAdvertising();
                pCharacteristic->setValue("ADVERTISING_RESTARTED");
                pCharacteristic->notify();
                return;
            }

            uint8_t newState = atoi(value.c_str());
            Serial.printf("Parsed state: %d\n", newState);

            if (newState <= 2) {
                unsigned long commandStartTime = millis();

                Serial.printf("Setting currentState to: %d\n", newState);
                currentState = newState;
                interruptSequence = true;
                stateChanged = true;

                // Notify servo task
                if (servoTaskHandle != NULL) {
                    Serial.println("Notifying servo task...");
                    xTaskNotifyGive(servoTaskHandle);
                } else {
                    Serial.println("ERROR: servoTaskHandle is NULL!");
                }

                // Send acknowledgment
                char response[32];
                snprintf(response, sizeof(response), "STATE_CHANGED:%d", currentState);
                pCharacteristic->setValue(response);
                pCharacteristic->notify();
                Serial.printf("Sent acknowledgment: %s\n", response);

                // Log movement command (non-blocking)
                unsigned long responseTime = millis() - commandStartTime;
                publishMovementCommand(value.c_str(), responseTime);
            } else {
                Serial.printf("Invalid state: %d (ignored)\n", newState);
            }
        }
    }
};

void resetServos() {
    servo1.write(SERVO_MIN);
    servo2.write(SERVO_MIN);
    servo3.write(SERVO_MIN);
    vTaskDelay(pdMS_TO_TICKS(500)); // Allow servos to reach position
}

void executeSequentialMovement() {
    Serial.println("Executing sequential movement");

    for (int cycle = 0; cycle < 3 && !interruptSequence; cycle++) {
        // Move servos to max position one by one
        servo1.write(SERVO_MAX);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        servo2.write(SERVO_MAX);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        servo3.write(SERVO_MAX);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        // Return servos to min position one by one
        servo1.write(SERVO_MIN);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        servo2.write(SERVO_MIN);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        servo3.write(SERVO_MIN);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;
    }
}

void executeSimultaneousMovement() {
    Serial.println("Executing simultaneous movement (all servos together)");

    for (int cycle = 0; cycle < 3 && !interruptSequence; cycle++) {
        Serial.printf("Simultaneous cycle %d/3\n", cycle + 1);

        // Move all servos to max position simultaneously
        servo1.write(SERVO_MAX);
        servo2.write(SERVO_MAX);
        servo3.write(SERVO_MAX);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;

        // Move all servos to min position simultaneously
        servo1.write(SERVO_MIN);
        servo2.write(SERVO_MIN);
        servo3.write(SERVO_MIN);
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        if (interruptSequence) break;
    }
}

void servoTask(void *pvParameters) {
    Serial.println("Servo task started");

    for (;;) {
        // Wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        interruptSequence = false;

        Serial.printf("Processing state: %d\n", currentState);

        // Always reset servos first
        resetServos();

        switch (currentState) {
            case 0:
                Serial.println("State 0: Idle - servos at rest position");
                break;

            case 1:
                executeSequentialMovement();
                break;

            case 2:
                executeSimultaneousMovement();
                break;

            default:
                Serial.println("Unknown state");
                break;
        }

        // Send completion notification if not interrupted
        if (!interruptSequence && deviceConnected && pCharacteristic) {
            char response[32];
            snprintf(response, sizeof(response), "STATE_COMPLETE:%d", currentState);
            pCharacteristic->setValue(response);
            pCharacteristic->notify();
        }

        Serial.printf("State %d execution finished\n", currentState);
    }
}

void initializeBLE() {
    Serial.println("Initializing BLE...");

    NimBLEDevice::init(DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::NOTIFY
    );

    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pCharacteristic->setValue("READY");

    pService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x0006);  // 7.5ms
    pAdvertising->setMaxPreferred(0x0012);  // 22.5ms

    // Set advertising data
    pAdvertising->setName(DEVICE_NAME);
    pAdvertising->setAppearance(0x00);

    // Start advertising
    NimBLEDevice::startAdvertising();
    Serial.println("BLE advertising started with improved parameters");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Servo Controller Starting ===");

    // Initialize servos
    Serial.println("Initializing servos...");
    servo1.attach(SERVO_PIN_1);
    servo2.attach(SERVO_PIN_2);
    servo3.attach(SERVO_PIN_3);

    // Set initial positions
    resetServos();

    // Initialize WiFi (non-blocking)
    initializeWiFi();

    // Initialize time synchronization (after WiFi)
    // Note: This will be called again once WiFi connects

    // Initialize MQTT
    initializeMQTT();

    // Initialize BLE
    initializeBLE();

    // Create servo control task
    xTaskCreatePinnedToCore(
        servoTask,
        "ServoTask",
        4096,
        NULL,
        2,
        &servoTaskHandle,
        1
    );

    Serial.println("=== Setup Complete ===");
    Serial.println("Waiting for BLE connections...");
    Serial.println("WiFi and MQTT will connect in background...");
}

void loop() {
    // Handle WiFi connection (non-blocking)
    checkWiFiConnection();

    // Handle MQTT connection (non-blocking)
    checkMQTTConnection();

    // Publish system status periodically
    if (millis() - lastStatusReport > STATUS_REPORT_INTERVAL) {
        publishSystemStatus();
        lastStatusReport = millis();
    }

    // Print status every 10 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        Serial.printf("Status: BLE=%s, WiFi=%s, MQTT=%s, State=%d\n",
                     deviceConnected ? "Connected" : "Disconnected",
                     wifiConnected ? "Connected" : "Disconnected",
                     mqttConnected ? "Connected" : "Disconnected",
                     currentState);
        lastStatus = millis();
    }

    // Small delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(100));
}
