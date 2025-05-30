# ESP32 Rehabilitation Hand Exoskeleton

A **professionally architected** Bluetooth Low Energy (BLE) controlled servo system using ESP32 and React Native mobile app. Designed for rehabilitation hand exoskeleton demonstrations with multiple movement patterns, power optimization, and enterprise-level code organization. This is a direct upgrade/fork from the [Stroke-Rehabilitation-Glove](https://github.com/h-jpj/Stroke-Rehabilitation-Glove).

## 🏗️ **Professional Modular Architecture**

This project features a **complete modular redesign** that transforms a monolithic 600+ line main.cpp into a clean, maintainable, enterprise-level codebase:

### **📁 Code Organization**
```
src/
├── main.cpp                    # ✨ Simplified main file (70 lines)
├── config/
│   └── Config.h               # 🔧 Centralized configuration
├── utils/
│   ├── Logger.h/.cpp          # 📝 Professional logging system
│   ├── TimeManager.h/.cpp     # ⏰ NTP sync and timestamps
│   └── ErrorHandler.h/.cpp    # 🛡️ Error handling and recovery
├── network/
│   ├── WiFiManager.h/.cpp     # 📶 WiFi connection management
│   └── MQTTManager.h/.cpp     # 📡 MQTT publishing and connection
├── bluetooth/
│   └── BLEManager.h/.cpp      # 🔵 BLE server and command handling
├── hardware/
│   ├── ServoController.h/.cpp # 🦾 Servo control with FreeRTOS
│   └── SystemMonitor.h/.cpp   # 📊 System health monitoring
└── app/
    ├── CommandProcessor.h/.cpp # ⚙️ Command validation and processing
    └── DeviceManager.h/.cpp    # 🎯 Main application coordinator
```

### **🌟 Architecture Benefits**
- **✅ Maintainable**: Each module has a single, clear responsibility
- **✅ Testable**: Components can be unit tested independently
- **✅ Scalable**: Easy to add new features without touching existing code
- **✅ Professional**: Enterprise-level code organization and error handling
- **✅ Robust**: Comprehensive logging, monitoring, and recovery mechanisms
- **✅ Performance**: Real-time monitoring with FreeRTOS task management

### **🔄 Before vs After**
| **Before (Monolithic)** | **After (Modular)** |
|-------------------------|---------------------|
| 600+ lines in main.cpp | 70 lines in main.cpp |
| All code in one file | 10 specialized modules |
| Basic error handling | Comprehensive error recovery |
| Simple logging | Multi-level logging system |
| Manual connection management | Automatic reconnection with health monitoring |
| Mixed responsibilities | Clear separation of concerns |

### **🚀 Key Features**
- **Servo Control**: Precise control with FreeRTOS task management and safety features
- **Dual Connectivity**: BLE for mobile control + WiFi/MQTT for data logging
- **Session Management**: Automatic session tracking with manual end session support
- **Real-time Data Logging**: Complete therapy session tracking with accurate duration calculation
- **Web Dashboard**: Professional browser-based interface with real-time updates
- **Cycle Tracking**: Accurate movement cycle counting and progress monitoring
- **Professional Logging**: Multi-level logging with timestamps and error tracking
- **Robust Error Handling**: Automatic recovery and system health monitoring
- **Command Processing**: Validation, queuing, rate limiting, and execution tracking
- **Security**: Comprehensive credential management and secure communication

## Hardware Requirements

- ESP32 Development Board (ESP32-WROOM-32 or similar)
- 3x Servo Motors (miuzei ms18 micro 9g servos or similar)
- USB cable for power and programming
- Jumper wires for connections

## 🔧 Wiring Diagram

| Component | ESP32 Pin |
|-----------|-----------|
| Servo 1 Signal | GPIO 19 |
| Servo 2 Signal | GPIO 22 |
| Servo 3 Signal | GPIO 23 |
| All Servo Power | Vin pin |
| All Servo Ground | GND pin |

## 🖨️ **3D Printed Hand Exoskeleton**

This project uses a **modified version** of the 3D printable hand exoskeleton design from Thingiverse:

**📎 Original Design**: [Thingiverse - Hand Exoskeleton](https://www.thingiverse.com/thing:2782111/files)

### **🔧 Modifications Made**
- **Servo Mount Holes**: Modified to fit **miuzei micro servo ms18** specifications
- **Improved Fit**: Adjusted mounting points for better servo alignment
- **Optimized Assembly**: Enhanced for easier servo installation and maintenance

### **📁 STL Files**
> **⚠️ Important Note**: We are **not currently hosting** the modified STL files.
>
> To use this project:
> 1. Download the original files from the [Thingiverse link](https://www.thingiverse.com/thing:2782111/files) above
> 2. Modify the servo mount holes to fit the miuzei ms18 servo dimensions (or whatever servos you are using)
> 3. Print using standard PLA settings (0.2mm layer height recommended)

### **🔩 Hardware Requirements**
- **3x miuzei ms18 micro servos** (9g micro servos)
- **M2/M3 screws** for servo mounting (check original design specifications)
- **Flexible filament** recommended for finger contact points (optional)
- **Standard PLA** for main structure components

⚠️ **Power Note**: This wiring is suitable for short-term demo purposes only. For production use, consider external power supply for servos.

## 🚀 Quick Start

### Prerequisites

**Software Requirements:**
- **Node.js** (v16 or later) - [Download here](https://nodejs.org/)
- **VS Code** with PlatformIO extension - [Get VS Code](https://code.visualstudio.com/)
- **Android Studio** (for Android development) - [Download here](https://developer.android.com/studio)
- **Git** - [Install Git](https://git-scm.com/)

**Hardware Requirements:**
- ESP32 development board
- 3x servo motors (miuzei ms18 micro 9g or similar)
- USB cable for programming
- Jumper wires

### Clone the Repository

```bash
git clone https://github.com/your-username/ESP32-Rehab-Hand-Exoskeleton.git
cd ESP32-Rehab-Hand-Exoskeleton
```

### 🔒 Security Configuration

**⚠️ IMPORTANT: Configure your credentials before building**

1. **Copy environment template files:**
   ```bash
   cp .env.example .env
   cp src/config.h.example src/config.h
   ```

2. **Update with your actual credentials:**
   ```bash
   # Edit .env with your server and database settings
   nano .env

   # Edit config.h with your WiFi and MQTT credentials
   nano src/config.h
   ```

3. **Security Notes:**
   - Never commit `.env` or `src/config.h` to version control
   - Use strong passwords for production deployments
   - Ensure your WiFi network uses WPA2/WPA3 encryption
   - Change default database credentials before deployment

## Software Setup

### ESP32 Firmware

1. **Install PlatformIO**:
   - Install VS Code
   - Install PlatformIO extension

2. **Build and Upload**:
   ```bash
   # From the project root directory
   pio run --target upload
   pio device monitor
   ```

3. **Expected Output**:
   ```
   === ESP32 Rehabilitation Hand Exoskeleton ===
   Firmware Version: 1.0.0
   Device ID: ESP32_001
   Starting system initialization...

   Phase 1: Initializing Foundation...
   Logger initialized
   TimeManager initialized
   ErrorHandler initialized
   Foundation initialization complete

   Phase 2: Initializing Communication...
   WiFiManager initialized
   MQTTManager initialized
   BLEManager initialized successfully
   Communication initialization complete

   Phase 3: Initializing Hardware...
   ServoController initialized successfully
   SystemMonitor initialized
   Hardware initialization complete

   Phase 4: Initializing Application...
   CommandProcessor initialized
   Application initialization complete

   === System Ready ===
   Device is ready to accept commands
   Available interfaces: BLE, WiFi/MQTT
   Send commands: 0 (home), 1 (sequential), 2 (simultaneous)
   ```

### Mobile App

1. **Install Dependencies**:
   ```bash
   cd ServoController
   npm install
   ```

2. **Build and Run**:
   ```bash
   # Build release APK for Android (Expo will handle everything)
   npx expo run:android --variant release
   ```

   **Note**: First run will take longer as Expo downloads Android SDK components and builds the project.

3. **Connection Process**:
   - If ESP32 was previously paired, forget it in Android Bluetooth settings
   - Open the mobile app
   - Tap "📡 Scan for ESP32"
   - Once connected, use the control buttons

## 🐳 **Docker Infrastructure**

This project includes a complete backend infrastructure using Docker containers for professional data logging and monitoring:

- **🦟 Mosquitto MQTT Broker**: Handles real-time data communication from ESP32
- **🗄️ MariaDB Database**: Stores therapy sessions, movement data, and system logs
- **🌐 Web Dashboard**: Browser-based interface for data visualization and monitoring
- **🔧 phpMyAdmin**: Database management interface for development

The Docker setup provides:
- **Real-time data logging** from ESP32 via WiFi/MQTT with synchronized timestamps
- **Professional web interface** accessible from any browser
- **Comprehensive session tracking** and progress monitoring
- **Scalable architecture** ready for multiple devices

**⏰ Time Synchronization**: The system uses Unix timestamps for accurate data correlation across devices. The ESP32 automatically synchronizes with NTP servers when WiFi connects, ensuring all logged events have precise timestamps for medical documentation and progress tracking.

**📋 Quick Setup**: Deploy all services with a single `docker-compose up -d` command.

**📖 Detailed Instructions**: See [Server Setup Guide](SERVER_SETUP.md) for complete installation and configuration steps.

## 🎮 Usage Instructions

### Control Modes

- **🔄 Run Servo Test**: Test individual servo with 3x 90° movements
- **📋 Sequential Movement**: Servos move one at a time (3 cycles)
- **⚡ Simultaneous Movement**: All servos move together (3 cycles)
- **⏹️ Stop All Servos**: Return to idle state (0° position)

### Mobile App Features

1. **Connection**: Tap "📡 Scan for ESP32" to find and connect
2. **Session Management**: Automatic session start on connection
3. **Control**: Use the control buttons to trigger movements
4. **Manual Session End**: "End Session" button for proper session termination
5. **Status**: Real-time connection status and command feedback
6. **Debug**: "🔍 Debug Info" button for troubleshooting

### 📡 BLE Communication Protocol

- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Commands**:
  - `"TEST"` - Run servo test
  - `"0"` - Stop/idle state
  - `"1"` - Sequential movement
  - `"2"` - Simultaneous movement
  - `"END_SESSION"` - Manually end current session
- **Responses**:
  - `STATE_CHANGED:X` - Command acknowledged
  - `TEST_COMPLETE` - Test sequence finished

## Power Management Features

- **Staggered Movements**: Reduces peak current draw
- **Interruptible Sequences**: Immediate response to new commands
- **Recovery Delays**: Prevents servo overheating
- **Single-Core Servo Task**: Dedicated FreeRTOS task for servo control

## 🔧 Troubleshooting

### Connection Issues
- **Can't Find ESP32**:
  - Ensure ESP32 is powered and shows "BLE advertising started"
  - Forget previous Bluetooth pairings in Android settings
  - Grant all Bluetooth permissions to the app
- **Connection Drops**:
  - Check power supply stability
  - Restart both ESP32 and mobile app
  - Use "🔍 Debug Info" to check connection state

### Hardware Issues
- **Servos Not Moving**:
  - Verify wiring connections (GPIO 19, 22, 23)
  - Check power supply (USB 3.0 recommended)
  - Monitor serial output for error messages
  - Check ServoController module status in logs
- **Jerky Movement**:
  - Insufficient power - try powered USB hub
  - Check servo connections
  - Review SystemMonitor health reports

### App Issues
- **Buttons Not Working**:
  - Ensure proper BLE connection established
  - Check console logs for error messages
  - Try "🔄 Run Servo Test" first to verify connection

## 👨‍💻 **Development Workflow**

The modular architecture enables efficient development and maintenance:

### **Adding New Features**
1. **Identify the appropriate module** (or create a new one)
2. **Implement the feature** within the module's scope
3. **Add logging and error handling** using the existing utilities
4. **Update the DeviceManager** if coordination is needed
5. **Test the module independently** before integration

### **Debugging and Troubleshooting**
- **Centralized Logging**: All modules use the same logging system
- **Component Isolation**: Issues can be traced to specific modules
- **Health Monitoring**: System automatically reports component status
- **Performance Tracking**: Built-in metrics help identify bottlenecks

### **Code Maintenance**
- **Single Responsibility**: Each module has one clear purpose
- **Minimal Dependencies**: Modules are loosely coupled
- **Clear Interfaces**: Well-defined APIs between components
- **Comprehensive Documentation**: Each module is self-documenting

## 📖 Detailed Documentation

- [Getting Started Guide](GETTING_STARTED.md) - Complete setup instructions
- [Hardware Setup](HARDWARE_SETUP.md) - Wiring diagrams and connections
- [Server Setup Guide](SERVER_SETUP.md) - MQTT broker and database infrastructure
- [Project Structure](PROJECT_STRUCTURE.md) - Code organization and architecture
- [Changelog](CHANGELOG.md) - Version history and updates

## Demo Script

1. **Setup**: Connect hardware, upload firmware, start mobile app
2. **Connection**: Demonstrate BLE pairing process
3. **State 0**: Show idle position
4. **State 1**: Demonstrate sequential movement pattern
5. **State 2**: Show staggered movement for power optimization
6. **Interruption**: Show immediate response to state changes
7. **Status**: Highlight real-time feedback system

## 🔧 Technical Details

### **Modular Architecture Implementation**
- **DeviceManager**: Central coordinator managing all subsystems
- **Component Isolation**: Each module handles its own initialization and lifecycle
- **Clean Interfaces**: Well-defined APIs between modules
- **Dependency Injection**: Modules communicate through callbacks and interfaces

### **FreeRTOS Implementation**
- **ServoController**: Dedicated FreeRTOS task for servo movements
- **Task Notifications**: Immediate response to movement commands
- **Thread-Safe Communication**: Safe inter-task communication patterns
- **Performance Monitoring**: Real-time loop time and resource tracking

### **Professional Logging System**
- **Multi-Level Logging**: DEBUG, INFO, WARNING, ERROR levels
- **Timestamped Entries**: Precise timing for debugging and monitoring
- **Memory Efficient**: Configurable log levels to optimize performance
- **System Integration**: Automatic logging of errors, state changes, and performance metrics

### **Error Handling & Recovery**
- **Comprehensive Error Tracking**: Centralized error reporting and history
- **Automatic Recovery**: Self-healing mechanisms for common failures
- **Health Monitoring**: Continuous system health assessment
- **Graceful Degradation**: System continues operating even with component failures

### **Network Management**
- **WiFiManager**: Automatic connection, reconnection, and network scanning
- **MQTTManager**: Robust MQTT publishing with retry logic and connection management
- **BLEManager**: Professional BLE server implementation with connection callbacks
- **Connection Resilience**: Automatic recovery from network interruptions

### **Command Processing**
- **Validation Pipeline**: Multi-stage command validation and sanitization
- **Rate Limiting**: Protection against command flooding
- **Queue Management**: Command queuing for busy periods
- **Execution Tracking**: Complete audit trail of command execution

### **System Monitoring**
- **Real-time Metrics**: Memory usage, CPU performance, network status
- **Health Assessment**: Automated system health scoring
- **Performance Tracking**: Loop time monitoring and optimization alerts
- **Resource Management**: Proactive memory and performance management

## Future Enhancements

- External power supply integration
- Position feedback sensors
- Force control algorithms
- Multi-device support
- ✅ **Data logging capabilities** (implemented via MQTT + MariaDB)
- ✅ **Web dashboard for data visualization** (implemented with real-time updates)
- ✅ **Session management with cycle tracking** (implemented)
- ✅ **Manual session termination** (implemented)
- iOS mobile app support
- GraphDB integration for advanced analytics

## 🔒 Security & Privacy

### Data Protection
- **Local Processing**: All data stays on your local network
- **No Cloud Dependencies**: Complete offline operation capability
- **Encrypted Communication**: WiFi WPA2/WPA3 encryption
- **Access Control**: Database and MQTT authentication required

### Credential Management
- **Environment Files**: Sensitive data in `.env` and `config.h` (not committed)
- **Strong Passwords**: Minimum 12 characters recommended
- **Regular Updates**: Change credentials periodically
- **Network Security**: Use secure WiFi networks only

### Files Never Committed to Git
```
.env                   # Server and database credentials
src/config.h           # WiFi and MQTT credentials
mariadb_data/          # Database files
mosquitto_data/        # MQTT broker data
```

## Safety Notes

⚠️ **Important**:
This is a demonstration project. Not certified for medical use nor is it intended for proper use.
