#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Servo.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device connected - keeping connection stable");
        // Don't stop advertising immediately - let connection stabilize first
        delay(100);
        NimBLEDevice::getAdvertising()->stop();
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        // Restart advertising after disconnect
        delay(1000); // Longer delay before restarting
        NimBLEDevice::startAdvertising();
        Serial.println("Advertising restarted after disconnect");
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

    // SIMPLE TEST: Move servo 3 (pin 23) back and forth
    Serial.println("=== SERVO TEST STARTING ===");
    Serial.println("Testing servo on pin 23...");

    for (int i = 0; i < 3; i++) {
        Serial.println("Moving servo to 90 degrees...");
        servo3.write(90);
        delay(1000);

        Serial.println("Moving servo to 0 degrees...");
        servo3.write(0);
        delay(1000);
    }

    Serial.println("=== SERVO TEST COMPLETE ===");
    Serial.println("If servo moved, your wiring is correct!");

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
}

void loop() {
    // Main loop handles system maintenance
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Print status every 10 seconds
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        Serial.printf("Status: %s, State: %d\n",
                     deviceConnected ? "Connected" : "Disconnected",
                     currentState);
        lastStatus = millis();
    }
}
