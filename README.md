# ESP32 Rehabilitation Hand Exoskeleton

A Bluetooth Low Energy (BLE) controlled servo system using ESP32 and React Native mobile app. Designed for rehabilitation hand exoskeleton demonstrations with multiple movement patterns and power optimization.

## Hardware Requirements

- ESP32 Development Board (ESP32-WROOM-32 or similar)
- 3x Servo Motors (miuzei ms18 micro 9g servos or similar)
- USB cable for power and programming
- Jumper wires for connections

## 🔧 Wiring Diagram

| Component | ESP32 Pin |
|-----------|-----------|
| Servo 1 Signal | GPIO 21 |
| Servo 2 Signal | GPIO 22 |
| Servo 3 Signal | GPIO 23 |
| All Servo Power | Vin pin |
| All Servo Ground | GND pin |

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
   === ESP32 Servo Controller Starting ===
   Initializing servos...
   Initializing BLE...
   BLE advertising started
   === Setup Complete ===
   Waiting for BLE connections...
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
- **Real-time data logging** from ESP32 via WiFi/MQTT
- **Professional web interface** accessible from any browser
- **Comprehensive session tracking** and progress monitoring
- **Scalable architecture** ready for multiple devices

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
2. **Control**: Use the control buttons to trigger movements
3. **Status**: Real-time connection status and command feedback
4. **Debug**: "🔍 Debug Info" button for troubleshooting

### 📡 BLE Communication Protocol

- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Commands**:
  - `"TEST"` - Run servo test
  - `"0"` - Stop/idle state
  - `"1"` - Sequential movement
  - `"2"` - Simultaneous movement
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
  - Verify wiring connections (GPIO 18, 19, 21)
  - Check power supply (USB 3.0 recommended)
  - Monitor serial output for error messages
- **Jerky Movement**:
  - Insufficient power - try powered USB hub
  - Check servo connections

### App Issues
- **Buttons Not Working**:
  - Ensure proper BLE connection established
  - Check console logs for error messages
  - Try "🔄 Run Servo Test" first to verify connection

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

## Technical Details

### FreeRTOS Implementation
- Main loop handles BLE communication
- Dedicated servo task on Core 1
- Task notifications for immediate response
- Interrupt-safe state management

### Power Optimization
- Sequential servo activation
- Configurable movement delays
- Staggered timing patterns
- Current draw reduction strategies

### BLE Architecture
- NimBLE stack for efficiency
- Custom service and characteristic
- Bidirectional communication
- Connection state management

## Future Enhancements

- External power supply integration
- Position feedback sensors
- Force control algorithms
- Multi-device support
- ✅ **Data logging capabilities** (implemented via MQTT + MariaDB)
- Web dashboard for data visualization
- iOS mobile app support
- GraphDB integration for advanced analytics

## Safety Notes

⚠️ **Important**: This is a demonstration project. For actual rehabilitation use:
- Implement proper safety interlocks
- Add emergency stop functionality
- Use medical-grade components
- Conduct proper testing and validation
- Ensure regulatory compliance
