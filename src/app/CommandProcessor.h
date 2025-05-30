#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>

enum class CommandSource {
    BLE,
    MQTT,
    SERIAL_PORT,
    INTERNAL
};

enum class CommandType {
    MOVEMENT,
    SYSTEM,
    CONFIGURATION,
    DIAGNOSTIC,
    UNKNOWN
};

struct Command {
    String rawCommand;
    CommandType type;
    CommandSource source;
    int commandCode;
    String parameters;
    unsigned long timestamp;
    bool isValid;
};

class CommandProcessor {
public:
    // Initialization
    void initialize();

    // Command processing
    Command parseCommand(const String& rawCommand, CommandSource source);
    bool validateCommand(const Command& command);
    bool executeCommand(const Command& command);

    // Command queue management
    bool queueCommand(const String& rawCommand, CommandSource source);
    bool hasQueuedCommands();
    Command getNextCommand();
    void clearCommandQueue();
    int getQueueSize();

    // Command history
    void recordCommand(const Command& command, bool success);
    Command getLastCommand();
    int getCommandCount();
    int getSuccessfulCommandCount();
    int getFailedCommandCount();

    // Rate limiting
    void setRateLimit(int commandsPerSecond);
    bool isRateLimited(CommandSource source);

    // Callbacks
    void setCommandCallback(void (*callback)(const Command& command, bool success));
    void setErrorCallback(void (*callback)(const Command& command, const String& error));

    // Statistics
    unsigned long getLastCommandTime();
    float getCommandSuccessRate();
    void logStatistics();

    // Utility methods (public for DeviceManager access)
    String commandSourceToString(CommandSource source);
    String commandTypeToString(CommandType type);

private:
    bool initialized;

    // Command queue
    static const int MAX_QUEUE_SIZE = 10;
    Command commandQueue[MAX_QUEUE_SIZE];
    int queueHead;
    int queueTail;
    int queueSize;

    // Command history
    static const int MAX_HISTORY_SIZE = 20;
    Command commandHistory[MAX_HISTORY_SIZE];
    int historyIndex;
    int historyCount;

    // Statistics
    int totalCommands;
    int successfulCommands;
    int failedCommands;
    unsigned long lastCommandTime;

    // Rate limiting
    int rateLimit;  // Commands per second
    unsigned long lastCommandTimes[4];  // One for each CommandSource

    // Callbacks
    void (*commandCallback)(const Command& command, bool success);
    void (*errorCallback)(const Command& command, const String& error);

    // Internal methods
    CommandType determineCommandType(const String& command);
    bool validateMovementCommand(int commandCode);
    bool validateSystemCommand(const String& command);
    bool executeMovementCommand(const Command& command);
    bool executeSystemCommand(const Command& command);
    bool executeConfigurationCommand(const Command& command);
    bool executeDiagnosticCommand(const Command& command);

    // Queue management
    bool isQueueFull();
    bool isQueueEmpty();
    void addToQueue(const Command& command);
    Command removeFromQueue();

    // History management
    void addToHistory(const Command& command, bool success);

    // Rate limiting
    bool checkRateLimit(CommandSource source);
    void updateRateLimit(CommandSource source);

    // Validation helpers
    bool isValidCommandCode(int code);
    bool isValidParameter(const String& param);

    // Logging
    void logCommand(const Command& command, bool success);
    void logError(const Command& command, const String& error);

    // Utility (moved to public section above)
};

#endif
