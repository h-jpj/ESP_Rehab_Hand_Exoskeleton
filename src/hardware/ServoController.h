#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

enum class ServoState {
    IDLE = 0,
    SEQUENTIAL_MOVEMENT = 1,
    SIMULTANEOUS_MOVEMENT = 2,
    HOMING = 3,
    ERROR = 4
};

enum class MovementType {
    NONE,
    SEQUENTIAL,
    SIMULTANEOUS,
    HOME
};

struct ServoStatus {
    int currentAngle;
    int targetAngle;
    bool isMoving;
    unsigned long lastMoveTime;
};

class ServoController {
public:
    // Initialization and lifecycle
    void initialize();
    void update();
    void shutdown();

    // Command execution
    bool executeCommand(const String& command);
    bool executeCommand(int commandCode);
    void stopAllMovement();
    void emergencyStop();

    // Movement patterns
    void executeSequentialMovement();
    void executeSimultaneousMovement();
    void returnToHome();

    // Status queries
    bool isBusy();
    ServoState getCurrentState();
    MovementType getCurrentMovementType();
    int getCompletedCycles();
    int getTotalCycles();
    float getMovementProgress();

    // Servo information
    ServoStatus getServoStatus(int servoIndex);
    int getServoCount();
    bool isServoAttached(int servoIndex);

    // Configuration
    void setMovementSpeed(int delayMs);
    void setCycleCount(int cycles);
    void setAngleRange(int minAngle, int maxAngle);

    // Callbacks
    void setMovementCompleteCallback(void (*callback)(ServoState state, int cycles));
    void setStateChangeCallback(void (*callback)(ServoState oldState, ServoState newState));

    // Statistics
    unsigned long getMovementStartTime();
    unsigned long getTotalMovementTime();
    int getMovementCount();

    // Enhanced analytics structures
    struct MovementMetrics {
        unsigned long startTime;
        unsigned long duration;
        bool successful;
        int servoIndex;
        int startAngle;
        int targetAngle;
        int actualAngle;
        float smoothness;
        String movementType;
        String sessionId;
    };

    struct ServoPerformance {
        int totalMovements;
        int successfulMovements;
        unsigned long totalTime;
        unsigned long averageTime;
        float successRate;
        float averageSmoothness;
        unsigned long lastMovementTime;
    };

    // Enhanced analytics methods
    MovementMetrics getLastMovementMetrics();
    ServoPerformance getServoPerformance(int servoIndex);
    void recordMovementMetrics(int servoIndex, unsigned long startTime, unsigned long duration,
                              bool successful, int startAngle, int targetAngle, int actualAngle,
                              float smoothness, const String& sessionId = "");
    void publishMovementAnalytics(const MovementMetrics& metrics);
    float calculateMovementSmoothness(int servoIndex, unsigned long duration);
    void resetPerformanceMetrics();

private:
    Servo servos[3];
    ServoStatus servoStatus[3];

    ServoState currentState;
    MovementType currentMovementType;
    bool initialized;
    bool movementInProgress;
    int currentCycle;
    int totalCycles;
    int movementDelayMs;
    int minAngle;
    int maxAngle;

    unsigned long movementStartTime;
    unsigned long totalMovementTime;
    int movementCount;

    // Enhanced analytics data
    MovementMetrics lastMovementMetrics;
    ServoPerformance servoPerformance[3];
    unsigned long individualServoStartTimes[3];
    int previousServoAngles[3];

    // Task management
    TaskHandle_t servoTaskHandle;
    static void servoTask(void* parameter);

    // Callbacks
    void (*movementCompleteCallback)(ServoState state, int cycles);
    void (*stateChangeCallback)(ServoState oldState, ServoState newState);

    // Movement execution
    void performSequentialCycles();
    void performSimultaneousCycles();
    void moveServoSmoothly(int servoIndex, int targetAngle, int delayMs);
    void moveAllServosTo(int angle, int delayMs);

    // State management
    void setState(ServoState newState);
    void resetMovementState();
    void updateServoStatus(int servoIndex, int angle, bool isMoving);

    // Validation and safety
    bool validateCommand(const String& command);
    bool validateAngle(int angle);
    bool isValidServoIndex(int index);

    // Logging and notifications
    void logMovementStart(MovementType type);
    void logMovementComplete(MovementType type, int cycles);
    void logStateChange(ServoState oldState, ServoState newState);

    // Configuration constants
    static const int SERVO_COUNT = 3;
    static const int DEFAULT_MIN_ANGLE = 0;
    static const int DEFAULT_MAX_ANGLE = 90;
    static const int DEFAULT_MOVEMENT_DELAY = 1000;
    static const int DEFAULT_CYCLES = 3;
    static const int TASK_STACK_SIZE = 4096;
    static const int TASK_PRIORITY = 1;
};

#endif
