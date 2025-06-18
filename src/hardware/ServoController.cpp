#include "ServoController.h"
#include "../config/Config.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "FreeRTOSManager.h"

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
    hasNewMetrics = false;

    // Initialize servo status
    for (int i = 0; i < SERVO_COUNT; i++) {
        servoStatus[i] = {0, 0, false, 0};
    }

    // Initialize analytics data
    resetPerformanceMetrics();
    for (int i = 0; i < SERVO_COUNT; i++) {
        previousServoAngles[i] = minAngle; // Start with home position
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

        // Create servo task through FreeRTOS Manager for proper coordination
        if (!createTaskThroughManager()) {
            Logger::error("Failed to create servo task through FreeRTOS Manager");
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

    // Delete task through FreeRTOS Manager
    destroyTaskThroughManager();

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

    // Notify task to start movement using FreeRTOS Manager coordination
    if (servoTaskHandle != nullptr) {
        // Notify servo task to start movement
        xTaskNotifyGive(servoTaskHandle);
        Logger::debug("Servo task notified for sequential movement");
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

    // Notify task to start movement using FreeRTOS Manager coordination
    if (servoTaskHandle != nullptr) {
        // Notify servo task to start movement
        xTaskNotifyGive(servoTaskHandle);
        Logger::debug("Servo task notified for simultaneous movement");
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

// =============================================================================
// FREERTOS MANAGER INTEGRATION
// =============================================================================

bool ServoController::createTaskThroughManager() {
    Logger::info("Creating servo task through FreeRTOS Manager...");

    // Ensure FreeRTOS Manager is initialized
    if (!FreeRTOSManager::isInitialized()) {
        Logger::error("FreeRTOS Manager not initialized - cannot create servo task");
        return false;
    }

    // Create task with FreeRTOS Manager coordination
    BaseType_t result = xTaskCreatePinnedToCore(
        servoTask,
        "ServoControl",  // Use consistent naming with FreeRTOS Manager
        TASK_STACK_SERVO_CONTROL,  // Use FreeRTOS Manager stack size
        this,
        PRIORITY_SERVO_CONTROL,    // Use FreeRTOS Manager priority
        &servoTaskHandle,
        CORE_APPLICATION  // Core 1 for application tasks
    );

    if (result != pdPASS) {
        Logger::error("Failed to create servo task");
        return false;
    }

    // Register task with FreeRTOS Manager
    FreeRTOSManager::setServoControlTask(servoTaskHandle);

    Logger::infof("Servo task created successfully with handle: %p", servoTaskHandle);
    return true;
}

void ServoController::destroyTaskThroughManager() {
    if (servoTaskHandle != nullptr) {
        Logger::info("Destroying servo task through FreeRTOS Manager...");

        // Unregister from FreeRTOS Manager
        FreeRTOSManager::setServoControlTask(nullptr);

        // Delete the task
        vTaskDelete(servoTaskHandle);
        servoTaskHandle = nullptr;

        Logger::info("Servo task destroyed successfully");
    }
}

// Static task function
void ServoController::servoTask(void* parameter) {
    ServoController* controller = static_cast<ServoController*>(parameter);

    Logger::info("Servo task started with FreeRTOS Manager coordination");

    while (true) {
        // Wait for notification to start movement
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        Logger::debug("Servo task received movement notification");

        // Perform movement based on type
        if (controller->currentMovementType == MovementType::SEQUENTIAL) {
            Logger::debug("Executing sequential movement cycles");
            controller->performSequentialCycles();
        } else if (controller->currentMovementType == MovementType::SIMULTANEOUS) {
            Logger::debug("Executing simultaneous movement cycles");
            controller->performSimultaneousCycles();
        }

        // Movement completed successfully
        Logger::debug("Servo movement cycles completed");

        // Feed watchdog to indicate task is healthy
        FreeRTOSManager::feedTaskWatchdog(xTaskGetCurrentTaskHandle());

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

    // Mark movement as complete and reset state
    movementInProgress = false;
    setState(ServoState::IDLE);

    // Sequential movement completed - state reset handled by servo task
}

void ServoController::performSimultaneousCycles() {
    Logger::infof("Executing simultaneous movement (%d cycles)", totalCycles);

    for (currentCycle = 1; currentCycle <= totalCycles; currentCycle++) {
        Logger::infof("Simultaneous cycle %d/%d", currentCycle, totalCycles);

        if (!movementInProgress) break;  // Check for stop condition

        // Move all servos together - FreeRTOS style
        moveAllServosTo(maxAngle, movementDelayMs);
        vTaskDelay(pdMS_TO_TICKS(movementDelayMs));  // FreeRTOS delay
        moveAllServosTo(minAngle, movementDelayMs);
        vTaskDelay(pdMS_TO_TICKS(movementDelayMs));  // FreeRTOS delay
    }

    Logger::infof("Simultaneous movement finished (%d cycles completed)", currentCycle - 1);

    // Mark movement as complete and reset state
    movementInProgress = false;
    setState(ServoState::IDLE);

    // Simultaneous movement completed - state reset handled by servo task
}

void ServoController::moveServoSmoothly(int servoIndex, int targetAngle, int delayMs) {
    if (!isValidServoIndex(servoIndex) || !validateAngle(targetAngle)) {
        return;
    }

    // Record movement start time and previous angle for analytics
    unsigned long startTime = millis();
    int startAngle = previousServoAngles[servoIndex];
    individualServoStartTimes[servoIndex] = startTime;

    servos[servoIndex].write(targetAngle);
    updateServoStatus(servoIndex, targetAngle, true);

    Logger::debugf("Servo %d moving to %d degrees", servoIndex, targetAngle);

    // Wait for movement to complete - FreeRTOS style
    vTaskDelay(pdMS_TO_TICKS(delayMs));

    // Calculate movement metrics
    unsigned long duration = millis() - startTime;
    int actualAngle = targetAngle; // Assume successful movement (no feedback sensor)
    bool successful = (actualAngle == targetAngle);
    float smoothness = calculateMovementSmoothness(servoIndex, duration);

    // Record the movement metrics
    recordMovementMetrics(servoIndex, startTime, duration, successful,
                         startAngle, targetAngle, actualAngle, smoothness);

    // Update previous angle for next movement
    previousServoAngles[servoIndex] = targetAngle;
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

// Enhanced analytics methods implementation
ServoController::MovementMetrics ServoController::getLastMovementMetrics() {
    return lastMovementMetrics;
}

ServoController::ServoPerformance ServoController::getServoPerformance(int servoIndex) {
    if (isValidServoIndex(servoIndex)) {
        return servoPerformance[servoIndex];
    }
    return {}; // Return empty struct for invalid index
}

void ServoController::recordMovementMetrics(int servoIndex, unsigned long startTime, unsigned long duration,
                                          bool successful, int startAngle, int targetAngle, int actualAngle,
                                          float smoothness, const String& sessionId) {
    if (!isValidServoIndex(servoIndex)) return;

    // Update last movement metrics
    lastMovementMetrics.startTime = startTime;
    lastMovementMetrics.duration = duration;
    lastMovementMetrics.successful = successful;
    lastMovementMetrics.servoIndex = servoIndex;
    lastMovementMetrics.startAngle = startAngle;
    lastMovementMetrics.targetAngle = targetAngle;
    lastMovementMetrics.actualAngle = actualAngle;
    lastMovementMetrics.smoothness = smoothness;
    lastMovementMetrics.sessionId = sessionId;

    // Determine movement type
    if (currentMovementType == MovementType::SEQUENTIAL) {
        lastMovementMetrics.movementType = "sequential";
    } else if (currentMovementType == MovementType::SIMULTANEOUS) {
        lastMovementMetrics.movementType = "simultaneous";
    } else if (currentMovementType == MovementType::HOME) {
        lastMovementMetrics.movementType = "home";
    } else {
        lastMovementMetrics.movementType = "unknown";
    }

    // Update servo performance metrics
    ServoPerformance& perf = servoPerformance[servoIndex];
    perf.totalMovements++;
    if (successful) {
        perf.successfulMovements++;
    }
    perf.totalTime += duration;
    perf.averageTime = perf.totalTime / perf.totalMovements;
    perf.successRate = (float)perf.successfulMovements / perf.totalMovements * 100.0f;
    perf.averageSmoothness = (perf.averageSmoothness * (perf.totalMovements - 1) + smoothness) / perf.totalMovements;
    perf.lastMovementTime = startTime + duration;

    Logger::debugf("Movement metrics recorded: Servo %d, Duration %lu ms, Success: %s, Smoothness: %.2f",
                   servoIndex, duration, successful ? "Yes" : "No", smoothness);

    // Publish analytics data
    publishMovementAnalytics(lastMovementMetrics);
}

void ServoController::publishMovementAnalytics(const MovementMetrics& metrics) {
    // Log the analytics data
    Logger::infof("Analytics: Servo %d, Type: %s, Duration: %lu ms, Success: %s, Smoothness: %.2f",
                  metrics.servoIndex, metrics.movementType.c_str(), metrics.duration,
                  metrics.successful ? "Yes" : "No", metrics.smoothness);

    // Store metrics for DeviceManager to publish
    // DeviceManager will call getLastMovementMetrics() to get this data
    lastMovementMetrics = metrics;
    hasNewMetrics = true;
}

float ServoController::calculateMovementSmoothness(int servoIndex, unsigned long duration) {
    if (!isValidServoIndex(servoIndex) || duration == 0) {
        return 0.0f;
    }

    // Simple smoothness calculation based on movement duration
    // Longer duration for same angle change = smoother movement
    // This is a basic implementation - can be enhanced with actual acceleration data
    float expectedDuration = 1000.0f; // Expected duration for smooth movement (1 second)
    float smoothness = (duration >= expectedDuration) ? 100.0f : (duration / expectedDuration) * 100.0f;

    // Cap at 100%
    return (smoothness > 100.0f) ? 100.0f : smoothness;
}

void ServoController::resetPerformanceMetrics() {
    // Reset all servo performance metrics
    for (int i = 0; i < SERVO_COUNT; i++) {
        servoPerformance[i] = {};
        individualServoStartTimes[i] = 0;
        previousServoAngles[i] = 0;
    }

    // Reset last movement metrics
    lastMovementMetrics = {};
    hasNewMetrics = false;

    Logger::info("Performance metrics reset");
}

bool ServoController::hasNewAnalytics() {
    return hasNewMetrics;
}

void ServoController::clearNewAnalytics() {
    hasNewMetrics = false;
}
