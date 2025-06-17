# 🦾 ESP32 Rehabilitation Hand Exoskeleton
## FreeRTOS-Based Rehabilitation Device

**[PLEASE REMEMBER TO READ THE SERVER_SETUP](SERVER_SETUP.md)**

**ESP32-based rehabilitation device with FreeRTOS multitasking architecture, real-time analytics, and comprehensive monitoring systems.**

## 🎯 **System Overview**

A rehabilitation device platform combining:
- **🔧 Hardware**: ESP32 dual-core controller with servo-driven hand exoskeleton and biometric monitoring
- **❤️ Biometrics**: Real-time heart rate and SpO2 monitoring with GY-MAX30102 sensor
- **📱 Mobile**: React Native app with BLE connectivity for device control
- **🌐 Backend**: Docker infrastructure with MQTT, MariaDB, and real-time analytics
- **📊 Analytics**: Clinical monitoring with Grafana dashboards and biometric data
- **🚀 Architecture**: FreeRTOS multitasking system with dual-core task distribution

## ✨ **Key Features**

### **🎮 Rehabilitation Control**
- **3-Servo Hand Exoskeleton**: Precise finger movement control for therapy
- **Multiple Movement Patterns**: Sequential, simultaneous, and test modes
- **Safety Systems**: Immediate stop capability and power management
- **Real-time Feedback**: Instant response to patient commands

### **❤️ Biometric Monitoring**
- **Heart Rate Monitoring**: Real-time pulse detection with GY-MAX30102 sensor
- **SpO2 Measurement**: Blood oxygen saturation monitoring
- **Signal Quality Assessment**: Automatic finger detection and signal validation
- **Real-time Display**: Live biometric data in web dashboard with hospital-style formatting

### **📱 Mobile Application**
- **React Native App**: Cross-platform mobile control interface
- **BLE Connectivity**: Wireless communication with ESP32 device
- **Session Management**: Automatic session tracking and manual termination
- **Real-time Status**: Live connection and command feedback

### **🏥 Clinical Analytics**
- **Real-time Monitoring**: Movement quality assessment and progress tracking
- **Biometric Integration**: Heart rate and SpO2 data collection during therapy sessions
- **Clinical Insights**: Quantitative rehabilitation progress measurement with vital signs
- **Session Analytics**: Therapy session analysis with biometric correlation
- **Performance Metrics**: System health and reliability monitoring

### **🚀 System Architecture**
- **FreeRTOS Multitasking**: Dual-core concurrent processing
- **Modular Design**: Component isolation and task-based architecture
- **System Monitoring**: Health, network, and performance tracking
- **Error Recovery**: Network reconnection and error handling

## 🏆 **Project Showcase**

### **🎯 System Evolution**
This project evolved from a simple Arduino-style polling system to a FreeRTOS-based multitasking architecture designed for rehabilitation applications.

### **📊 Technical Features**
- **🏗️ FreeRTOS Architecture**: 7-task system with dual-core task distribution
- **⚡ Improved Performance**: Better responsiveness through concurrent processing
- **🔒 System Reliability**: Task isolation and error handling mechanisms
- **📈 Clinical Analytics**: Real-time movement assessment and progress tracking
- **🌐 Backend Infrastructure**: Docker-based system with MQTT, MariaDB, and Grafana

### **💼 Key Features**
- **Real-time Analytics**: Movement quality assessment and progress tracking
- **System Monitoring**: Health, performance, and network status monitoring
- **Error Recovery**: Network reconnection and connection handling
- **Modular Design**: Extensible for additional sensors and features
- **Documentation**: Technical documentation and setup guides

### **🎓 Learning Aspects**
Demonstrates:
- **Embedded Systems Design**: FreeRTOS architecture and task management
- **IoT Development**: Device connectivity and data pipeline implementation
- **IoT System Integration**: Device-to-cloud data flow
- **Real-time Processing**: Clinical data collection and analysis
- **System Documentation**: Technical project presentation

## 🏗️ **FreeRTOS Architecture**

### **🎯 Dual-Core Task Distribution**

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                           ESP32 DUAL-CORE ARCHITECTURE                      ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  CORE 0 (Protocol CPU)              │  CORE 1 (Application CPU)             ║
║  ═══════════════════════             │  ══════════════════════════            ║
║                                      │                                        ║
║  Priority 5: WiFi Manager Task      │  Priority 6: Servo Control Task       ║
║  ├─ 4KB Stack                       │  ├─ 4KB Stack                         ║
║  ├─ Network connectivity            │  ├─ Safety-critical servo control     ║
║  └─ 1 second cycle                  │  └─ Real-time movement execution      ║
║                                      │                                        ║
║  Priority 4: MQTT Publisher Task    │  Priority 5: I2C Manager Task         ║
║  ├─ 6KB Stack                       │  ├─ 3KB Stack                         ║
║  ├─ Outgoing message queue          │  ├─ Sensor communication              ║
║  └─ 100Hz publishing rate           │  └─ Hardware interface                ║
║                                      │                                        ║
║  Priority 4: MQTT Subscriber Task   │  Priority 2: Session Analytics Task   ║
║  ├─ 4KB Stack                       │  ├─ 4KB Stack                         ║
║  ├─ Incoming message handling       │  ├─ Real-time analytics processing    ║
║  └─ 20Hz connection management      │  └─ Clinical progress tracking        ║
║                                      │                                        ║
║  Priority 3: BLE Server Task        │  Priority 1: System Health Task       ║
║  ├─ 8KB Stack                       │  ├─ 3KB Stack                         ║
║  ├─ Mobile app connectivity         │  ├─ Background health monitoring      ║
║  └─ 10Hz connection management      │  └─ 5 second health checks            ║
║                                      │                                        ║
║  Priority 2: Network Watchdog Task  │  Priority 3: Heart Rate Task          ║
║  ├─ 2KB Stack                       │  ├─ 4KB Stack                         ║
║  ├─ Network monitoring & recovery   │  ├─ GY-MAX30102 sensor management    ║
║  └─ 10 second monitoring cycle      │  └─ Real-time biometric monitoring    ║
║                                      │                                        ║
║  TOTAL: 24KB Stack (96KB RAM)       │  TOTAL: 18KB Stack (72KB RAM)         ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### **📊 System Benefits**
- **⚡ Concurrent Processing**: Components run independently without blocking
- **🎯 Core Distribution**: Protocol tasks on Core 0, Application tasks on Core 1
- **🔒 Task Isolation**: Component failures are contained to individual tasks
- **📈 Priority Scheduling**: Critical tasks receive appropriate CPU time
- **🔄 Error Recovery**: Network issues are handled automatically

### **🗂️ Code Structure**

```
src/
├── main.cpp                    # Main application entry point
├── config/Config.h             # Centralized configuration
├── utils/                      # Logging, time sync, error handling
├── network/                    # WiFi, MQTT, and Network Watchdog management
│   ├── WiFiManager             # Dedicated WiFi task (Core 0, Priority 5)
│   ├── MQTTManager             # Publisher/Subscriber tasks (Core 0, Priority 4)
│   └── NetworkWatchdogManager  # Network monitoring task (Core 0, Priority 2)
├── bluetooth/BLEManager        # BLE server task (Core 0, Priority 3)
├── hardware/                   # Hardware control and monitoring
│   ├── ServoController         # Servo control task (Core 1, Priority 6)
│   ├── SystemMonitor           # Legacy system monitoring
│   └── I2CManager              # I2C communication task (Core 1, Priority 5)
├── health/                     # System health monitoring
│   └── SystemHealthManager     # Health monitoring task (Core 1, Priority 1)
├── sensors/                    # Sensor management and data collection
│   └── HeartRateManager        # Heart rate monitoring task (Core 1, Priority 3)
├── analytics/                  # Real-time analytics processing
│   └── SessionAnalyticsManager # Analytics task (Core 1, Priority 2)
├── session/SessionManager      # Session management and tracking
├── command/CommandProcessor    # Command validation and processing
└── app/DeviceManager           # System coordination and integration
```

## Hardware Requirements

### **Core Components**
- ESP32 Development Board (ESP32-WROOM-32 or similar)
- 3x Servo Motors (miuzei ms18 micro 9g servos or similar)
- GY-MAX30102 Heart Rate Pulse Oximetry Sensor (implemented)
- USB cable for power and programming
- Jumper wires for connections

### **Biometric Monitoring (Implemented)**
- **GY-MAX30102 Heart Rate Sensor**: Real-time pulse and SpO2 monitoring
  - I2C communication (SCL: GPIO 21, SDA: GPIO 18, INT: GPIO 4)
  - Finger detection with IR threshold > 50,000
  - Real-time SpO2 calculation using R-value algorithm
  - Hospital-style data display in web dashboard

### **Future Enhancement Sensors (Optional)**
- **Pressure Sensors**: 20g~2kg High Resistance Type Thin Film Pressure Sensor for force measurement
- **Motion Tracking**: GY-521 MPU-6050 (3-axis gyroscope + 3-axis accelerometer) for movement analysis

*Note: Future sensor integration will use InfluxDB for high-frequency data storage alongside MariaDB for session management.*

## 📊 **Real-time Analytics & Monitoring**

### **🏥 Clinical Analytics**
- **Movement Quality Assessment**: Real-time scoring based on smoothness, timing, and success rate
- **Progress Tracking**: Quantitative improvement measurement across sessions
- **Clinical Insights**: Professional rehabilitation progress indicators
- **Trend Analysis**: Historical data analysis with improvement tracking

### **⚡ Real-time Data Pipeline**
```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Servo Data    │───▶│  DeviceManager   │───▶│  MQTT Broker    │
│   - Movements   │    │  - Analytics     │    │  - Individual   │
│   - Quality     │    │  - Integration   │    │  - Quality      │
│   - Performance │    │  - Publishing    │    │  - Progress     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  System Health  │───▶│  Health Manager  │───▶│   Database      │
│  - Memory       │    │  - Monitoring    │    │  - Events       │
│  - Performance  │    │  - Alerts        │    │  - Sessions     │
│  - Network      │    │  - Reporting     │    │  - Analytics    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ Session Events  │───▶│ Analytics Manager│───▶│   Webapp        │
│ - Start/End     │    │ - Real-time      │    │ - Real-time     │
│ - Movements     │    │ - Clinical       │    │ - Dashboards    │
│ - Progress      │    │ - Quality        │    │ - Grafana       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### **📈 Monitoring Capabilities**
- **System Health**: Memory usage, CPU performance, task monitoring
- **Network Resilience**: Automatic WiFi/MQTT/BLE recovery
- **Performance Tracking**: Loop times, response times, reliability metrics
- **Clinical Data**: Movement quality, session progress, improvement trends

## 🔧 Circuit Diagram & Wiring

![Circuit Diagram](circuit_diagram.svg)

*Professional circuit diagram showing complete system connections - see [WIRING_REFERENCE.md](WIRING_REFERENCE.md) for detailed assembly guide*

### Connection Summary

### Servo Connections
| Component | ESP32 Pin |
|-----------|-----------|
| Servo 1 Signal | GPIO 19 |
| Servo 2 Signal | GPIO 22 |
| Servo 3 Signal | GPIO 23 |
| All Servo Power | Vin pin |
| All Servo Ground | GND pin |

### Heart Rate Sensor (GY-MAX30102)
| Component | ESP32 Pin |
|-----------|-----------|
| SCL | GPIO 21 |
| SDA | GPIO 18 |
| INT | GPIO 4 |
| VCC | 3.3V |
| GND | GND |

## 🖨️ 3D Printed Components

### STL Files
- **📁 STLs/**: Contains 3D printable files for the ESP32 mounting base but not the hand exoskeleton itself.
- **Hand Exoskeleton**: This is what the design is based on [Thingiverse design](https://www.thingiverse.com/thing:2782111/files) with servo mount holes adjusted for miuzei ms18 servos however you'll have to make the modifactions yourself... I may have lost the files I altered when University switched their systems. Hard lesson learned.
- **ESP32 Base**: Custom mounting base that attaches ESP32 to a splint for wearable use

### Print Settings
- **Material**: PLA (standard), flexible filament for finger contact points (optional)
- **Layer Height**: 0.2mm recommended
- **Infill**: 20-30% for structural components
- **Supports**: Required for overhangs

### Hardware
- 3x miuzei ms18 micro servos (9g)
- M3 screws for servo mounting
- ESP32 development board

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

3. **Expected Output** (FreeRTOS System Initialization):
   ```
   === ESP32 Rehabilitation Hand Exoskeleton ===
   Firmware Version: 2.0.0 (FreeRTOS Edition)
   Device ID: ESP32_001
   Starting FreeRTOS system initialization...

   Phase 1: Initializing Foundation...
   Logger initialized
   TimeManager initialized
   ErrorHandler initialized
   Foundation initialization complete

   Phase 2: Initializing Communication...
   WiFi Manager task started on Core 0
   MQTT Publisher and Subscriber tasks started on Core 0
   BLE Server task started on Core 0
   Network Watchdog task started on Core 0
   Communication initialization complete

   Phase 3: Initializing Hardware...
   Servo Control task started on Core 1
   I2C Manager task started on Core 1
   SystemMonitor initialized
   Hardware initialization complete

   Phase 4: Initializing Application...
   System Health task started on Core 1
   Session Analytics task started on Core 1
   CommandProcessor initialized
   Application initialization complete

   === FreeRTOS System Ready ===
   Architecture: Dual-core multitasking (7 active tasks)
   Core 0: WiFi, MQTT, BLE, Network Watchdog
   Core 1: Servo Control, I2C, Health, Analytics
   Available interfaces: BLE, WiFi/MQTT
   Real-time analytics: ENABLED
   Automatic recovery: ENABLED
   Send commands: 0 (home), 1 (sequential), 2 (simultaneous)

   Component Status:
   WiFi Manager: Connected (Task: Running)
   MQTT Manager: Connected (Tasks: Running)
   BLE Manager: Advertising (Task: Running)
   Network Watchdog Manager: Healthy (Task: Running)
   Servo Controller: Ready
   System Health Manager: Healthy (Task: Running)
   Session Analytics Manager: Ready (Task: Running)
   Session Manager: Idle

   System Health: EXCELLENT
   Memory Usage: 45.2% (152KB/336KB)
   Network Status: All connections healthy
   Analytics: Real-time processing active
   ```

### Mobile App

1. **Install Dependencies**:
   ```bash
   cd servo-controller
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

## 🐳 Docker Infrastructure

Complete backend infrastructure for data logging and monitoring:

- **MQTT Broker**: Real-time ESP32 communication
- **MariaDB**: Session and movement data storage
- **Web Dashboard**: Browser-based monitoring interface
- **Grafana**: Clinical analytics dashboards
- **phpMyAdmin**: Database management

**Setup**: `docker-compose up -d` - See [Server Setup Guide](SERVER_SETUP.md) for details.

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

## 📖 Documentation

- [Server Setup Guide](SERVER_SETUP.md) - MQTT broker and database infrastructure
- [Database Queries](DATABASE_QUERIES.md) - Complete reference for all queries and configurations

## Demo Script

1. **Setup**: Connect hardware, upload firmware, start mobile app
2. **Connection**: Demonstrate BLE pairing process
3. **State 0**: Show idle position
4. **State 1**: Demonstrate sequential movement pattern
5. **State 2**: Show staggered movement for power optimization
6. **Interruption**: Show immediate response to state changes
7. **Status**: Highlight real-time feedback system

## 🚀 **Technical Features**

### **🏗️ System Architecture**
- **FreeRTOS Design**: 7-task multithreading architecture across dual cores
- **Modular Design**: Clean separation of concerns with dedicated task modules
- **Component Isolation**: Independent task operation and error containment
- **Extensible Platform**: Designed for additional sensors and features

### **⚡ Performance Features**
- **Concurrent Processing**: Components run independently without blocking
- **Dual-Core Usage**: Task distribution across both ESP32 cores
- **Priority Scheduling**: Critical tasks receive appropriate CPU time
- **Predictable Execution**: Consistent timing and response characteristics

### **🔒 Reliability Features**
- **Task Isolation**: Component failures are contained to individual tasks
- **Error Recovery**: Network connections reconnect automatically
- **System Monitoring**: Health checks and performance tracking
- **Error Handling**: Logging and recovery mechanisms

### **📊 Analytics & Monitoring**
- **Real-time Analytics**: Movement quality and progress assessment
- **System Health**: Memory, performance, and network status tracking
- **Event Logging**: System event tracking and debugging information
- **Performance Metrics**: Built-in monitoring for system optimization

### **🌐 Network Management**
- **Multi-protocol Connectivity**: WiFi, MQTT, and BLE with automatic reconnection
- **Network Monitoring**: Connection health monitoring and recovery
- **Message Queuing**: Reliable inter-task communication
- **Connection Handling**: Maintains connectivity during network issues

## 🎯 **System Transformation Journey**

### **📈 System Evolution**

| **Aspect** | **Before (Arduino Style)** | **After (FreeRTOS)** | **Improvement** |
|---|---|---|---|
| **Architecture** | Single-threaded polling | 7-task multithreading | ⭐⭐⭐⭐⭐ |
| **Responsiveness** | Blocked by slow operations | Independent task execution | ⭐⭐⭐⭐⭐ |
| **Reliability** | Single point of failure | Isolated task failures | ⭐⭐⭐⭐⭐ |
| **Performance** | Variable timing | Deterministic scheduling | ⭐⭐⭐⭐⭐ |
| **Monitoring** | Basic logging | Comprehensive health tracking | ⭐⭐⭐⭐⭐ |
| **Recovery** | Manual intervention | Automatic recovery | ⭐⭐⭐⭐⭐ |
| **Scalability** | Difficult to extend | Modular task addition | ⭐⭐⭐⭐⭐ |
| **Analytics** | Basic data logging | Real-time clinical analytics | ⭐⭐⭐⭐⭐ |

### **🏆 Key Achievements**
- **✅ FreeRTOS Architecture**: Multitasking system with task isolation and scheduling
- **✅ Improved Performance**: Better responsiveness through concurrent processing
- **✅ Clinical Analytics**: Movement assessment and progress tracking capabilities
- **✅ Error Recovery**: Network reconnection and error handling mechanisms
- **✅ System Monitoring**: Health, performance, and network status tracking
- **✅ Modular Design**: Extensible architecture for additional sensors and features

### **🔮 Future Enhancements**

#### **Hardware Expansion**
- External power supply integration
- Position feedback sensors (encoders)
- Force control algorithms with pressure sensors
- Multi-device coordination and synchronization

#### **Software Features**
- iOS mobile app support
- Machine learning integration for movement prediction
- Advanced clinical reporting and EMR integration
- Cloud connectivity for remote monitoring

#### **Analytics Enhancement**
- GraphDB integration for advanced relationship analysis
- Predictive analytics for rehabilitation outcomes
- AI-powered movement optimization
- Clinical decision support systems
- InfluxDB for better real-time data management (maybe)

#### **✅ Completed Features**
- **Data logging capabilities** (implemented via MQTT + MariaDB)
- **Web dashboard for data visualization** (implemented with real-time updates)
- **Session management with cycle tracking** (implemented)
- **Manual session termination** (implemented)
- **Real-time analytics** (implemented with clinical insights)
- **System health monitoring** (implemented with comprehensive tracking)
- **Automatic recovery** (implemented for all network connections)

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

## 🎯 **Project Applications**

### **🏥 Potential Clinical Uses**
- **Rehabilitation Therapy**: Movement assessment and progress tracking
- **Research Applications**: Clinical data collection and analysis
- **Therapy Monitoring**: Session monitoring and feedback systems
- **Data Analytics**: Clinical insights and reporting capabilities

### **💼 Technical Applications**
- **Embedded System Design**: Example of FreeRTOS architecture implementation
- **IoT Development**: Device-to-cloud data pipeline demonstration
- **Real-time Systems**: FreeRTOS multitasking architecture example
- **Data Processing**: Real-time analytics and visualization

### **🎓 Learning Applications**
- **Embedded Systems**: FreeRTOS multitasking and task management
- **IoT Development**: Device connectivity and data pipeline implementation
- **System Integration**: Hardware-to-cloud data flow
- **System Design**: Modular architecture and documentation practices

### **🚀 Technical Improvements**
- **Performance**: Improved system responsiveness through concurrent processing
- **Architecture**: Dual-core FreeRTOS task distribution
- **Monitoring**: Health, performance, and network status tracking
- **Recovery**: Error handling and network reconnection mechanisms
- **Analytics**: Real-time data processing and clinical insights

---

## 📄 **License**

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 **Acknowledgments**

- **ESP32 Community** for excellent documentation and support
- **FreeRTOS** for professional real-time operating system
- **React Native Community** for mobile development resources
- **Docker Community** for containerization best practices
- **Medical Device Industry** for inspiring professional standards

---

**This project demonstrates the evolution from a simple Arduino sketch to a FreeRTOS-based multitasking system, showcasing embedded system design principles and real-time analytics implementation.**
