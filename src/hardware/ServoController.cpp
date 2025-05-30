#include "ServoController.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"

void ServoController::initialize() {
    if (initialized) return;

    Logger::info("Initializing Servo Controller...");

    currentState = ServoState::IDLE;
    currentMovementType = MovementType::NONE;
    movementInProgress = false;
    currentCycle = 0;
    totalCycles = DEFAULT_CYCLES;
    movementDelayMs = DEFAULT_MOVEMENT_DELAY;
    minAngle = DEFAULT_MIN_ANGLE;
    maxAngle = DEFAULT_MAX_ANGLE;
    movementStartTime = 0;
    totalMovementTime = 0;
    movementCount = 0;
    movementCompleteCallback = nullptr;
    stateChangeCallback = nullptr;

    // Initialize servo status
    for (int i = 0; i < SERVO_COUNT; i++) {
        servoStatus[i] = {0, 0, false, 0};
    }

    try {
        // Attach servos to pins
        for (int i = 0; i < SERVO_COUNT; i++) {
            int channel = servos[i].attach(SERVO_PINS[i]);
            if (channel < 0) {
                Logger::errorf("Failed to attach servo %d to pin %d", i, SERVO_PINS[i]);
                REPORT_ERROR(ErrorCode::SERVO_INITIALIZATION_FAILED,
                           "Servo attachment failed");
                return;
            }

            // Set initial position
            servos[i].write(minAngle);
            updateServoStatus(i, minAngle, false);

            Logger::infof("Servo %d attached to pin %d (channel %d)", i, SERVO_PINS[i], channel);
        }

        // Create servo task
        xTaskCreate(
            servoTask,
            "ServoTask",
            TASK_STACK_SIZE,
            this,
            TASK_PRIORITY,
            &servoTaskHandle
        );

        if (servoTaskHandle == nullptr) {
            Logger::error("Failed to create servo task");
            REPORT_ERROR(ErrorCode::SERVO_INITIALIZATION_FAILED, "Task creation failed");
            return;
        }

        initialized = true;
        Logger::info("Servo Controller initialized successfully");

    } catch (const std::exception& e) {
        Logger::errorf("Servo initialization exception: %s", e.what());
        REPORT_ERROR(ErrorCode::SERVO_INITIALIZATION_FAILED, "Initialization exception");
    }
}

void ServoController::update() {
    if (!initialized) return;

    // Update servo status timestamps
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (servoStatus[i].isMoving) {
            // Check if movement should be considered complete
            if (millis() - servoStatus[i].lastMoveTime > movementDelayMs + 100) {
                updateServoStatus(i, servoStatus[i].targetAngle, false);
            }
        }
    }
}

void ServoController::shutdown() {
    if (!initialized) return;

    Logger::info("Shutting down Servo Controller...");

    stopAllMovement();

    // Delete task
    if (servoTaskHandle != nullptr) {
        vTaskDelete(servoTaskHandle);
        servoTaskHandle = nullptr;
    }

    // Detach servos
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (servos[i].attached()) {
            servos[i].detach();
        }
    }

    initialized = false;
    Logger::info("Servo Controller shutdown complete");
}

bool ServoController::executeCommand(const String& command) {
    if (!validateCommand(command)) {
        REPORT_WARNING(ErrorCode::INVALID_COMMAND, "Invalid servo command: " + command);
        return false;
    }

    int commandCode = command.toInt();
    return executeCommand(commandCode);
}

bool ServoController::executeCommand(int commandCode) {
    if (!initialized || movementInProgress) {
        Logger::warning("Cannot execute command - servo controller busy or not initialized");
        return false;
    }

    Logger::infof("Executing servo command: %d", commandCode);

    switch (commandCode) {
        case 0:
            returnToHome();
            return true;

        case 1:
            executeSequentialMovement();
            return true;

        case 2:
            executeSimultaneousMovement();
            return true;

        default:
            Logger::warningf("Unknown servo command: %d", commandCode);
            REPORT_WARNING(ErrorCode::INVALID_COMMAND, "Unknown command code");
            return false;
    }
}

void ServoController::stopAllMovement() {
    if (!initialized) return;

    Logger::info("Stopping all servo movement");

    movementInProgress = false;
    setState(ServoState::IDLE);

    // Mark all servos as not moving
    for (int i = 0; i < SERVO_COUNT; i++) {
        updateServoStatus(i, servoStatus[i].currentAngle, false);
    }
}

void ServoController::emergencyStop() {
    Logger::warning("EMERGENCY STOP - All servo movement halted");

    stopAllMovement();

    // Move all servos to safe position (home)
    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].write(minAngle);
        updateServoStatus(i, minAngle, false);
    }

    setState(ServoState::ERROR);
}

void ServoController::executeSequentialMovement() {
    if (!initialized || movementInProgress) return;

    Logger::info("Starting sequential movement");
    logMovementStart(MovementType::SEQUENTIAL);

    currentMovementType = MovementType::SEQUENTIAL;
    setState(ServoState::SEQUENTIAL_MOVEMENT);
    movementInProgress = true;
    currentCycle = 0;
    movementStartTime = millis();
    movementCount++;

    // Notify task to start movement
    if (servoTaskHandle != nullptr) {
        xTaskNotifyGive(servoTaskHandle);
    }
}

void ServoController::executeSimultaneousMovement() {
    if (!initialized || movementInProgress) return;

    Logger::info("Starting simultaneous movement");
    logMovementStart(MovementType::SIMULTANEOUS);

    currentMovementType = MovementType::SIMULTANEOUS;
    setState(ServoState::SIMULTANEOUS_MOVEMENT);
    movementInProgress = true;
    currentCycle = 0;
    movementStartTime = millis();
    movementCount++;

    // Notify task to start movement
    if (servoTaskHandle != nullptr) {
        xTaskNotifyGive(servoTaskHandle);
    }
}

void ServoController::returnToHome() {
    if (!initialized) return;

    Logger::info("Returning servos to home position");

    currentMovementType = MovementType::HOME;
    setState(ServoState::HOMING);

    moveAllServosTo(minAngle, movementDelayMs);

    setState(ServoState::IDLE);
    currentMovementType = MovementType::NONE;
}

bool ServoController::isBusy() {
    return movementInProgress || currentState != ServoState::IDLE;
}

ServoState ServoController::getCurrentState() {
    return currentState;
}

MovementType ServoController::getCurrentMovementType() {
    return currentMovementType;
}

int ServoController::getCompletedCycles() {
    return currentCycle;
}

int ServoController::getTotalCycles() {
    return totalCycles;
}

float ServoController::getMovementProgress() {
    if (totalCycles == 0 || !movementInProgress) {
        return 0.0f;
    }

    return (float)currentCycle / (float)totalCycles;
}

ServoStatus ServoController::getServoStatus(int servoIndex) {
    if (isValidServoIndex(servoIndex)) {
        return servoStatus[servoIndex];
    }

    return {0, 0, false, 0};  // Invalid servo
}

int ServoController::getServoCount() {
    return SERVO_COUNT;
}

bool ServoController::isServoAttached(int servoIndex) {
    if (isValidServoIndex(servoIndex)) {
        return servos[servoIndex].attached();
    }
    return false;
}

void ServoController::setMovementSpeed(int delayMs) {
    if (delayMs >= 100 && delayMs <= 2000) {  // Reasonable range
        movementDelayMs = delayMs;
        Logger::infof("Movement speed set to %d ms", delayMs);
    } else {
        Logger::warning("Invalid movement speed - using default");
    }
}

void ServoController::setCycleCount(int cycles) {
    if (cycles >= 1 && cycles <= 10) {  // Reasonable range
        totalCycles = cycles;
        Logger::infof("Cycle count set to %d", cycles);
    } else {
        Logger::warning("Invalid cycle count - using default");
    }
}

void ServoController::setAngleRange(int minAngle, int maxAngle) {
    if (validateAngle(minAngle) && validateAngle(maxAngle) && minAngle < maxAngle) {
        this->minAngle = minAngle;
        this->maxAngle = maxAngle;
        Logger::infof("Angle range set to %d-%d degrees", minAngle, maxAngle);
    } else {
        Logger::warning("Invalid angle range - using default");
    }
}

void ServoController::setMovementCompleteCallback(void (*callback)(ServoState state, int cycles)) {
    movementCompleteCallback = callback;
}

void ServoController::setStateChangeCallback(void (*callback)(ServoState oldState, ServoState newState)) {
    stateChangeCallback = callback;
}

unsigned long ServoController::getMovementStartTime() {
    return movementStartTime;
}

unsigned long ServoController::getTotalMovementTime() {
    return totalMovementTime;
}

int ServoController::getMovementCount() {
    return movementCount;
}

// Static task function
void ServoController::servoTask(void* parameter) {
    ServoController* controller = static_cast<ServoController*>(parameter);

    Logger::info("Servo task started");

    while (true) {
        // Wait for notification to start movement
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (controller->currentMovementType == MovementType::SEQUENTIAL) {
            controller->performSequentialCycles();
        } else if (controller->currentMovementType == MovementType::SIMULTANEOUS) {
            controller->performSimultaneousCycles();
        }

        // Movement complete
        controller->movementInProgress = false;
        controller->setState(ServoState::IDLE);
        controller->currentMovementType = MovementType::NONE;

        unsigned long movementDuration = millis() - controller->movementStartTime;
        controller->totalMovementTime += movementDuration;

        controller->logMovementComplete(controller->currentMovementType, controller->currentCycle);

        // Notify completion via callback
        if (controller->movementCompleteCallback) {
            controller->movementCompleteCallback(controller->currentState, controller->currentCycle);
        }
    }
}

void ServoController::performSequentialCycles() {
    Logger::infof("Executing sequential movement (%d cycles)", totalCycles);

    for (currentCycle = 1; currentCycle <= totalCycles; currentCycle++) {
        Logger::infof("Sequential cycle %d/%d", currentCycle, totalCycles);

        if (!movementInProgress) break;  // Check for stop condition

        // Move each servo to max angle sequentially (servo1 -> servo2 -> servo3)
        for (int servo = 0; servo < SERVO_COUNT; servo++) {
            if (!movementInProgress) break;
            moveServoSmoothly(servo, maxAngle, movementDelayMs);
            delay(movementDelayMs);
        }

        // Move each servo back to min angle sequentially (servo1 -> servo2 -> servo3)
        for (int servo = 0; servo < SERVO_COUNT; servo++) {
            if (!movementInProgress) break;
            moveServoSmoothly(servo, minAngle, movementDelayMs);
            delay(movementDelayMs);
        }

        if (!movementInProgress) break;  // Movement was stopped
    }

    Logger::infof("Sequential movement finished (%d cycles completed)", currentCycle - 1);
}

void ServoController::performSimultaneousCycles() {
    Logger::infof("Executing simultaneous movement (%d cycles)", totalCycles);

    for (currentCycle = 1; currentCycle <= totalCycles; currentCycle++) {
        Logger::infof("Simultaneous cycle %d/%d", currentCycle, totalCycles);

        if (!movementInProgress) break;  // Check for stop condition

        // Move all servos together
        moveAllServosTo(maxAngle, movementDelayMs);
        delay(movementDelayMs);
        moveAllServosTo(minAngle, movementDelayMs);
        delay(movementDelayMs);
    }

    Logger::infof("Simultaneous movement finished (%d cycles completed)", currentCycle - 1);
}

void ServoController::moveServoSmoothly(int servoIndex, int targetAngle, int delayMs) {
    if (!isValidServoIndex(servoIndex) || !validateAngle(targetAngle)) {
        return;
    }

    servos[servoIndex].write(targetAngle);
    updateServoStatus(servoIndex, targetAngle, true);

    Logger::debugf("Servo %d moving to %d degrees", servoIndex, targetAngle);
}

void ServoController::moveAllServosTo(int angle, int delayMs) {
    if (!validateAngle(angle)) return;

    for (int i = 0; i < SERVO_COUNT; i++) {
        servos[i].write(angle);
        updateServoStatus(i, angle, true);
    }

    Logger::debugf("All servos moving to %d degrees", angle);
}

void ServoController::setState(ServoState newState) {
    if (newState != currentState) {
        ServoState oldState = currentState;
        currentState = newState;

        logStateChange(oldState, newState);

        if (stateChangeCallback) {
            stateChangeCallback(oldState, newState);
        }
    }
}

void ServoController::resetMovementState() {
    movementInProgress = false;
    currentCycle = 0;
    currentMovementType = MovementType::NONE;
    setState(ServoState::IDLE);
}

void ServoController::updateServoStatus(int servoIndex, int angle, bool isMoving) {
    if (isValidServoIndex(servoIndex)) {
        servoStatus[servoIndex].currentAngle = angle;
        servoStatus[servoIndex].targetAngle = angle;
        servoStatus[servoIndex].isMoving = isMoving;
        servoStatus[servoIndex].lastMoveTime = millis();
    }
}

bool ServoController::validateCommand(const String& command) {
    if (command.length() == 0) return false;

    int commandCode = command.toInt();
    return (commandCode >= 0 && commandCode <= 2);
}

bool ServoController::validateAngle(int angle) {
    return (angle >= 0 && angle <= 180);  // Standard servo range
}

bool ServoController::isValidServoIndex(int index) {
    return (index >= 0 && index < SERVO_COUNT);
}

void ServoController::logMovementStart(MovementType type) {
    switch (type) {
        case MovementType::SEQUENTIAL:
            Logger::info("Movement started: Sequential");
            break;
        case MovementType::SIMULTANEOUS:
            Logger::info("Movement started: Simultaneous");
            break;
        case MovementType::HOME:
            Logger::info("Movement started: Homing");
            break;
        default:
            break;
    }
}

void ServoController::logMovementComplete(MovementType type, int cycles) {
    switch (type) {
        case MovementType::SEQUENTIAL:
            Logger::infof("Movement complete: Sequential (%d cycles)", cycles);
            break;
        case MovementType::SIMULTANEOUS:
            Logger::infof("Movement complete: Simultaneous (%d cycles)", cycles);
            break;
        case MovementType::HOME:
            Logger::info("Movement complete: Homing");
            break;
        default:
            break;
    }
}

void ServoController::logStateChange(ServoState oldState, ServoState newState) {
    Logger::infof("Servo state changed: %d -> %d", (int)oldState, (int)newState);
}
