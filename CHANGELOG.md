# Changelog

## [2.1.0] - 2025-06-17

### 🩺 Biometric Monitoring Integration

#### Heart Rate Sensor Implementation
- **GY-MAX30102 Integration**: Real-time heart rate and SpO2 monitoring
- **I2C Communication**: Dedicated I2C task for sensor management
- **Signal Processing**:
  - Finger detection with IR threshold > 50,000
  - Real-time SpO2 calculation using R-value algorithm
  - Signal quality assessment and validation
- **FreeRTOS Task**: Dedicated Heart Rate task (Core 1, Priority 3)
- **Pin Configuration**:
  - SCL: GPIO 21
  - SDA: GPIO 18
  - INT: GPIO 4
  - VCC: 3.3V, GND: GND

#### Real-time Data Pipeline
- **MQTT Publishing**: Heart rate data published to `rehab_exo/ESP32_001/sensors/heart_rate`
- **WebSocket Integration**: Real-time biometric data streaming to web dashboard
- **Database Storage**: Biometric data stored in MariaDB with session correlation
- **Hospital-style Display**: Professional medical data formatting in web interface

#### Web Dashboard Enhancement
- **Biometric Display**: Real-time heart rate and SpO2 monitoring
- **Signal Quality Indicators**: Visual feedback for sensor status
- **Finger Detection**: Live finger presence indication
- **Trend Analysis**: Real-time biometric trend calculations
- **Session Integration**: Biometric data correlated with therapy sessions

#### System Architecture Updates
- **8-Task FreeRTOS System**: Added Heart Rate task to existing 7-task architecture
- **Dual-Core Distribution**: Heart Rate task runs on Core 1 with other application tasks
- **Memory Allocation**: Updated to 18KB stack on Core 1 (72KB total RAM)
- **Error Handling**: Comprehensive I2C error recovery and sensor validation

#### Documentation Updates
- **README.md**: Updated with biometric monitoring features and architecture
- **HARDWARE_SETUP.md**: Added GY-MAX30102 wiring and setup instructions
- **GETTING_STARTED.md**: Updated with heart rate sensor testing procedures
- **PROJECT_STRUCTURE.md**: Reflected current clean project structure

#### Technical Improvements
- **Real-time Processing**: Sub-second biometric data updates
- **Data Validation**: Automatic signal quality assessment
- **Session Analytics**: Biometric data integration with rehabilitation sessions
- **Professional Display**: Hospital-grade data presentation standards

### 🧹 Project Cleanup
- **Removed Development Artifacts**: Cleaned up 22 temporary files including phase implementations, test files, and outdated documentation
- **Streamlined Structure**: Organized project for professional presentation
- **Updated Documentation**: Comprehensive documentation reflecting current implementation

## [1.0.0] - 2024-12-19

### 🎉 Initial Release

#### ESP32 Firmware
- **BLE Server Implementation**: NimBLE-based Bluetooth Low Energy server
- **Servo Control**: Support for 3 servo motors with precise positioning
- **Movement Patterns**:
  - Sequential movement (servos move one at a time)
  - Simultaneous movement (all servos move together)
  - Individual servo test mode
- **FreeRTOS Integration**: Dedicated servo task for real-time control
- **Power Optimization**: Staggered movements to reduce current draw
- **Command Protocol**: String-based commands ("TEST", "0", "1", "2")

#### Mobile App (React Native + Expo)
- **BLE Scanner**: Automatic ESP32 device discovery
- **Connection Management**: Robust connection handling with auto-reconnect
- **Control Interface**: 
  - 🔄 Run Servo Test button
  - 📋 Sequential Movement button
  - ⚡ Simultaneous Movement button
  - ⏹️ Stop All Servos button
- **Real-time Status**: Connection status and command feedback
- **Debug Tools**: Comprehensive debugging information
- **User Experience**: Clear instructions and error handling

#### Hardware Support
- **ESP32 Development Board**: ESP32-WROOM-32 compatible
- **Servo Motors**: 3x miuzei ms18 micro 9g servos
- **Pin Configuration**:
  - Servo 1: GPIO 18
  - Servo 2: GPIO 19
  - Servo 3: GPIO 21
- **Power Supply**: USB-powered with Vin pin distribution

#### Communication Protocol
- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Bidirectional**: Commands from app, acknowledgments from ESP32
- **Base64 Encoding**: Reliable data transmission

#### Development Tools
- **PlatformIO**: ESP32 firmware development and upload
- **Expo**: React Native mobile app development
- **TypeScript**: Type-safe mobile app development
- **Serial Monitor**: Real-time ESP32 debugging

#### Documentation
- **README.md**: Comprehensive project overview
- **GETTING_STARTED.md**: Step-by-step setup guide
- **HARDWARE_SETUP.md**: Detailed wiring instructions
- **PROJECT_STRUCTURE.md**: Code organization guide
- **CHANGELOG.md**: This file

#### Key Features
- ✅ Wireless BLE control up to 10 meters range
- ✅ Sub-100ms command response time
- ✅ Power-optimized servo movements
- ✅ Interruptible movement sequences
- ✅ Real-time status feedback
- ✅ Robust error handling and recovery
- ✅ Cross-platform mobile app (Android tested)
- ✅ Comprehensive debugging tools

#### Testing & Validation
- **Hardware Testing**: Verified with miuzei ms18 servos
- **Power Testing**: Stable operation via USB 3.0 power
- **BLE Testing**: Reliable connection and command transmission
- **Mobile Testing**: Android 15 compatibility verified
- **Range Testing**: 10+ meter BLE range confirmed

#### Known Limitations
- **iOS Support**: Not tested (Android only)
- **Power Supply**: USB power limits simultaneous servo operation
- **Servo Count**: Fixed at 3 servos (expandable in code)
- **Safety Features**: Basic implementation (not medical-grade)

### 🔧 Technical Specifications
- **BLE Range**: ~10 meters (line of sight)
- **Command Latency**: <100ms
- **Movement Precision**: ±1° (servo dependent)
- **Power Consumption**: ~1.5A peak (all servos)
- **Operating Voltage**: 5V ±10%
- **Supported Platforms**: ESP32, Android

### 📦 Dependencies
#### ESP32
- ESP32Servo library
- NimBLE-Arduino library
- Arduino framework

#### Mobile App
- react-native-ble-plx
- expo framework
- TypeScript

### 🚀 Future Roadmap
- [ ] iOS mobile app support
- [ ] External power supply integration
- [ ] Position feedback sensors
- [ ] Force control algorithms
- [ ] Multi-device support
- [x] ~~Data logging capabilities~~ (✅ Completed - MQTT + MariaDB)
- [x] ~~Real-time biometric monitoring~~ (✅ Completed - GY-MAX30102)
- [x] ~~Web dashboard~~ (✅ Completed - Real-time webapp)
- [ ] Additional sensors (pressure, accelerometer)
- [ ] Safety interlock system
- [ ] Medical-grade compliance features
