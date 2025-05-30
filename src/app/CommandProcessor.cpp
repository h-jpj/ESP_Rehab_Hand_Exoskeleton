#include "CommandProcessor.h"
#include "../utils/Logger.h"
#include "../utils/ErrorHandler.h"
#include "../utils/TimeManager.h"

void CommandProcessor::initialize() {
    if (initialized) return;

    Logger::info("Initializing Command Processor...");

    // Initialize queue
    queueHead = 0;
    queueTail = 0;
    queueSize = 0;

    // Initialize history
    historyIndex = 0;
    historyCount = 0;

    // Initialize statistics
    totalCommands = 0;
    successfulCommands = 0;
    failedCommands = 0;
    lastCommandTime = 0;

    // Initialize rate limiting
    rateLimit = 5;  // 5 commands per second default
    for (int i = 0; i < 4; i++) {
        lastCommandTimes[i] = 0;
    }

    // Initialize callbacks
    commandCallback = nullptr;
    errorCallback = nullptr;

    initialized = true;
    Logger::info("Command Processor initialized");
}

Command CommandProcessor::parseCommand(const String& rawCommand, CommandSource source) {
    Command cmd;
    cmd.rawCommand = rawCommand;
    cmd.source = source;
    cmd.timestamp = TimeManager::getCurrentTimestamp();
    cmd.isValid = false;
    cmd.parameters = "";

    // Trim whitespace
    String trimmed = rawCommand;
    trimmed.trim();

    if (trimmed.length() == 0) {
        cmd.type = CommandType::UNKNOWN;
        cmd.commandCode = -1;
        return cmd;
    }

    // Determine command type
    cmd.type = determineCommandType(trimmed);

    // Parse command code for movement commands
    if (cmd.type == CommandType::MOVEMENT) {
        cmd.commandCode = trimmed.toInt();
        cmd.isValid = validateMovementCommand(cmd.commandCode);
    } else {
        cmd.commandCode = -1;
        cmd.isValid = validateCommand(cmd);
    }

    Logger::debugf("Parsed command: '%s' -> Type: %s, Code: %d, Valid: %s",
                  rawCommand.c_str(),
                  commandTypeToString(cmd.type).c_str(),
                  cmd.commandCode,
                  cmd.isValid ? "Yes" : "No");

    return cmd;
}

bool CommandProcessor::validateCommand(const Command& command) {
    switch (command.type) {
        case CommandType::MOVEMENT:
            return validateMovementCommand(command.commandCode);
        case CommandType::SYSTEM:
            return validateSystemCommand(command.rawCommand);
        case CommandType::CONFIGURATION:
        case CommandType::DIAGNOSTIC:
            return true;  // Basic validation for now
        default:
            return false;
    }
}

bool CommandProcessor::executeCommand(const Command& command) {
    if (!initialized || !command.isValid) {
        logError(command, "Invalid command");
        return false;
    }

    // Check rate limiting
    if (isRateLimited(command.source)) {
        logError(command, "Rate limited");
        return false;
    }

    bool success = false;
    unsigned long startTime = millis();

    try {
        switch (command.type) {
            case CommandType::MOVEMENT:
                success = executeMovementCommand(command);
                break;
            case CommandType::SYSTEM:
                success = executeSystemCommand(command);
                break;
            case CommandType::CONFIGURATION:
                success = executeConfigurationCommand(command);
                break;
            case CommandType::DIAGNOSTIC:
                success = executeDiagnosticCommand(command);
                break;
            default:
                logError(command, "Unknown command type");
                return false;
        }

        unsigned long executionTime = millis() - startTime;

        // Record command execution
        recordCommand(command, success);
        updateRateLimit(command.source);

        if (success) {
            Logger::infof("Command executed successfully in %lu ms: %s",
                         executionTime, command.rawCommand.c_str());
        } else {
            Logger::warningf("Command execution failed: %s", command.rawCommand.c_str());
        }

        // Notify via callback
        if (commandCallback) {
            commandCallback(command, success);
        }

    } catch (const std::exception& e) {
        Logger::errorf("Command execution exception: %s", e.what());
        logError(command, "Execution exception");
        success = false;
    }

    return success;
}

bool CommandProcessor::queueCommand(const String& rawCommand, CommandSource source) {
    if (isQueueFull()) {
        Logger::warning("Command queue full - dropping command");
        return false;
    }

    Command cmd = parseCommand(rawCommand, source);
    if (!cmd.isValid) {
        logError(cmd, "Invalid command - not queued");
        return false;
    }

    addToQueue(cmd);
    Logger::debugf("Command queued: %s (Queue size: %d)", rawCommand.c_str(), queueSize);

    return true;
}

bool CommandProcessor::hasQueuedCommands() {
    return !isQueueEmpty();
}

Command CommandProcessor::getNextCommand() {
    if (isQueueEmpty()) {
        return {};  // Return empty command
    }

    return removeFromQueue();
}

void CommandProcessor::clearCommandQueue() {
    queueHead = 0;
    queueTail = 0;
    queueSize = 0;
    Logger::info("Command queue cleared");
}

int CommandProcessor::getQueueSize() {
    return queueSize;
}

void CommandProcessor::recordCommand(const Command& command, bool success) {
    totalCommands++;
    lastCommandTime = millis();

    if (success) {
        successfulCommands++;
    } else {
        failedCommands++;
    }

    addToHistory(command, success);
    logCommand(command, success);
}

Command CommandProcessor::getLastCommand() {
    if (historyCount > 0) {
        int lastIndex = (historyIndex - 1 + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
        return commandHistory[lastIndex];
    }

    return {};  // Return empty command
}

int CommandProcessor::getCommandCount() {
    return totalCommands;
}

int CommandProcessor::getSuccessfulCommandCount() {
    return successfulCommands;
}

int CommandProcessor::getFailedCommandCount() {
    return failedCommands;
}

void CommandProcessor::setRateLimit(int commandsPerSecond) {
    if (commandsPerSecond > 0 && commandsPerSecond <= 100) {
        rateLimit = commandsPerSecond;
        Logger::infof("Rate limit set to %d commands/second", commandsPerSecond);
    } else {
        Logger::warning("Invalid rate limit - using default");
    }
}

bool CommandProcessor::isRateLimited(CommandSource source) {
    return !checkRateLimit(source);
}

void CommandProcessor::setCommandCallback(void (*callback)(const Command& command, bool success)) {
    commandCallback = callback;
}

void CommandProcessor::setErrorCallback(void (*callback)(const Command& command, const String& error)) {
    errorCallback = callback;
}

unsigned long CommandProcessor::getLastCommandTime() {
    return lastCommandTime;
}

float CommandProcessor::getCommandSuccessRate() {
    if (totalCommands == 0) return 0.0f;
    return ((float)successfulCommands / (float)totalCommands) * 100.0f;
}

void CommandProcessor::logStatistics() {
    Logger::infof("Command Statistics:");
    Logger::infof("  Total: %d", totalCommands);
    Logger::infof("  Successful: %d", successfulCommands);
    Logger::infof("  Failed: %d", failedCommands);
    Logger::infof("  Success Rate: %.1f%%", getCommandSuccessRate());
    Logger::infof("  Queue Size: %d", queueSize);
}

CommandType CommandProcessor::determineCommandType(const String& command) {
    // Check if it's a numeric movement command
    if (command.length() == 1 && isDigit(command.charAt(0))) {
        return CommandType::MOVEMENT;
    }

    // Check for system commands
    String lower = command;
    lower.toLowerCase();

    if (lower.startsWith("status") || lower.startsWith("info") ||
        lower.startsWith("restart") || lower.startsWith("reset") ||
        lower.startsWith("end_session") || lower == "end_session") {
        return CommandType::SYSTEM;
    }

    if (lower.startsWith("config") || lower.startsWith("set")) {
        return CommandType::CONFIGURATION;
    }

    if (lower.startsWith("test") || lower.startsWith("diag")) {
        return CommandType::DIAGNOSTIC;
    }

    return CommandType::UNKNOWN;
}

bool CommandProcessor::validateMovementCommand(int commandCode) {
    return (commandCode >= 0 && commandCode <= 2);
}

bool CommandProcessor::validateSystemCommand(const String& command) {
    // Basic validation for system commands
    return command.length() > 0 && command.length() < 50;
}

bool CommandProcessor::executeMovementCommand(const Command& command) {
    // This will be implemented by DeviceManager via callback
    // For now, just return true to indicate the command was processed
    Logger::infof("Movement command: %d", command.commandCode);
    return true;
}

bool CommandProcessor::executeSystemCommand(const Command& command) {
    String lower = command.rawCommand;
    lower.toLowerCase();

    if (lower == "status") {
        Logger::info("System status requested");
        return true;
    } else if (lower == "restart") {
        Logger::warning("System restart requested");
        return true;
    } else if (lower == "end_session") {
        Logger::info("Session end requested");
        return true;
    }

    return false;
}

bool CommandProcessor::executeConfigurationCommand(const Command& command) {
    Logger::infof("Configuration command: %s", command.rawCommand.c_str());
    return true;
}

bool CommandProcessor::executeDiagnosticCommand(const Command& command) {
    Logger::infof("Diagnostic command: %s", command.rawCommand.c_str());
    return true;
}

bool CommandProcessor::isQueueFull() {
    return queueSize >= MAX_QUEUE_SIZE;
}

bool CommandProcessor::isQueueEmpty() {
    return queueSize == 0;
}

void CommandProcessor::addToQueue(const Command& command) {
    if (!isQueueFull()) {
        commandQueue[queueTail] = command;
        queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
        queueSize++;
    }
}

Command CommandProcessor::removeFromQueue() {
    if (isQueueEmpty()) {
        return {};
    }

    Command cmd = commandQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    queueSize--;

    return cmd;
}

void CommandProcessor::addToHistory(const Command& command, bool success) {
    Command historyEntry = command;
    // Add success flag to history entry (we could extend Command struct for this)

    commandHistory[historyIndex] = historyEntry;
    historyIndex = (historyIndex + 1) % MAX_HISTORY_SIZE;

    if (historyCount < MAX_HISTORY_SIZE) {
        historyCount++;
    }
}

bool CommandProcessor::checkRateLimit(CommandSource source) {
    unsigned long now = millis();
    unsigned long interval = 1000 / rateLimit;  // Interval between commands in ms

    int sourceIndex = (int)source;
    if (sourceIndex < 0 || sourceIndex >= 4) return true;  // Allow if invalid source

    if (now - lastCommandTimes[sourceIndex] >= interval) {
        return true;
    }

    return false;
}

void CommandProcessor::updateRateLimit(CommandSource source) {
    int sourceIndex = (int)source;
    if (sourceIndex >= 0 && sourceIndex < 4) {
        lastCommandTimes[sourceIndex] = millis();
    }
}

void CommandProcessor::logCommand(const Command& command, bool success) {
    Logger::infof("Command %s: %s [%s] -> %s",
                 commandSourceToString(command.source).c_str(),
                 command.rawCommand.c_str(),
                 commandTypeToString(command.type).c_str(),
                 success ? "SUCCESS" : "FAILED");
}

void CommandProcessor::logError(const Command& command, const String& error) {
    Logger::errorf("Command error: %s - %s", command.rawCommand.c_str(), error.c_str());

    if (errorCallback) {
        errorCallback(command, error);
    }
}

String CommandProcessor::commandSourceToString(CommandSource source) {
    switch (source) {
        case CommandSource::BLE: return "BLE";
        case CommandSource::MQTT: return "MQTT";
        case CommandSource::SERIAL_PORT: return "SERIAL";
        case CommandSource::INTERNAL: return "INTERNAL";
        default: return "UNKNOWN";
    }
}

String CommandProcessor::commandTypeToString(CommandType type) {
    switch (type) {
        case CommandType::MOVEMENT: return "MOVEMENT";
        case CommandType::SYSTEM: return "SYSTEM";
        case CommandType::CONFIGURATION: return "CONFIG";
        case CommandType::DIAGNOSTIC: return "DIAGNOSTIC";
        default: return "UNKNOWN";
    }
}
