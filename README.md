# ESP32 Rehabilitation Hand Exoskeleton

**[PLEASE REMEMBER TO READ THE SERVER_SETUP](SERVER_SETUP.md)**

ESP32-based hand rehabilitation device with BLE mobile control, real-time data logging, and clinical analytics. Includes modular firmware, React Native app, and Docker infrastructure for professional therapy monitoring.

## Features

- **Servo Control**: 3-servo hand exoskeleton with sequential/simultaneous movement patterns
- **Mobile Control**: React Native app with BLE connectivity
- **Data Logging**: Real-time MQTT publishing to MariaDB database
- **Web Dashboard**: Browser-based monitoring with live updates
- **Analytics**: Grafana dashboards for clinical data visualization
- **Session Management**: Automatic tracking with manual session termination
- **Modular Architecture**: Clean, maintainable codebase with proper error handling

## Code Structure

```
src/
├── main.cpp                    # Main application entry point
├── config/Config.h             # Centralized configuration
├── utils/                      # Logging, time sync, error handling
├── network/                    # WiFi and MQTT management
├── bluetooth/BLEManager        # BLE server and command handling
├── hardware/                   # Servo control and system monitoring
└── app/                        # Command processing and device coordination
```

## Hardware Requirements

### **Core Components**
- ESP32 Development Board (ESP32-WROOM-32 or similar)
- 3x Servo Motors (miuzei ms18 micro 9g servos or similar)
- USB cable for power and programming
- Jumper wires for connections

### **Enhanced Analytics Sensors (Optional - Future Integration)**
- **Pressure Sensors**: 20g~2kg High Resistance Type Thin Film Pressure Sensor for force measurement
- **Motion Tracking**: GY-521 MPU-6050 (3-axis gyroscope + 3-axis accelerometer) for movement analysis
- **Biometric Monitoring**: GY-MAX30102 Heart Rate Pulse Oximetry Sensor for patient monitoring

*Note: Enhanced sensor integration will use InfluxDB for high-frequency data storage alongside MariaDB for session management.*

## 🔧 Wiring Diagram

| Component | ESP32 Pin |
|-----------|-----------|
| Servo 1 Signal | GPIO 19 |
| Servo 2 Signal | GPIO 22 |
| Servo 3 Signal | GPIO 23 |
| All Servo Power | Vin pin |
| All Servo Ground | GND pin |

## 🖨️ 3D Printed Components

### STL Files
- **📁 STLs/**: Contains 3D printable files for the hand exoskeleton and ESP32 mounting base
- **Hand Exoskeleton**: Modified version of [Thingiverse design](https://www.thingiverse.com/thing:2782111/files) with servo mount holes adjusted for miuzei ms18 servos
- **ESP32 Base**: Custom mounting base that attaches ESP32 to a splint for wearable use

### Print Settings
- **Material**: PLA (standard), flexible filament for finger contact points (optional)
- **Layer Height**: 0.2mm recommended
- **Infill**: 20-30% for structural components
- **Supports**: Required for overhangs

### Hardware
- 3x miuzei ms18 micro servos (9g)
- M2/M3 screws for servo mounting
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

## Technical Features

- **Modular Architecture**: Clean separation of concerns with dedicated modules
- **FreeRTOS**: Dedicated servo control task with real-time performance
- **Error Handling**: Comprehensive logging and automatic recovery
- **Network Management**: Robust WiFi/MQTT with automatic reconnection
- **Command Processing**: Validation, rate limiting, and execution tracking

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
