# Getting Started - ESP32 Servo Controller

## Quick Start Guide

This guide will get you up and running with the ESP32 servo controller in under 30 minutes.

## Prerequisites

### Software Requirements
- **VS Code** with PlatformIO extension
- **Node.js** (v16 or later)
- **Python 3** (for testing script)
- **Mobile device** with Bluetooth support

### Hardware Requirements
- ESP32 development board
- 3x servo motors (SG90 or similar)
- GY-MAX30102 Heart Rate Pulse Oximetry Sensor
- USB cable
- Jumper wires

## Step 1: Hardware Setup (5 minutes)

1. **Wire the components** according to the diagram in `HARDWARE_SETUP.md`:

   **Servos:**
   - Servo 1 signal → GPIO 19
   - Servo 2 signal → GPIO 22
   - Servo 3 signal → GPIO 23
   - All servo power → Vin
   - All servo ground → GND

   **Heart Rate Sensor (GY-MAX30102):**
   - SCL → GPIO 21
   - SDA → GPIO 18
   - INT → GPIO 4
   - VCC → 3.3V
   - GND → GND

2. **Connect ESP32** to your computer via USB

## Step 2: Software Setup (10 minutes)

### ESP32 Firmware
```bash
# Build and upload ESP32 firmware
pio run --target upload
pio device monitor  # Monitor serial output
```

### Mobile App
```bash
# Setup mobile app
cd servo-controller
npm install
npx expo run:android --variant release
```

## Step 3: Test the System (10 minutes)

### Test 1: ESP32 Serial Monitor
```bash
pio device monitor
```
Expected output:
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
Heart rate sensor detected at address 0x57
Hardware initialization complete

Phase 4: Initializing Application...
System Health task started on Core 1
Session Analytics task started on Core 1
Heart Rate task started on Core 1
Application initialization complete

=== FreeRTOS System Ready ===
Architecture: Dual-core multitasking (8 active tasks)
Core 0: WiFi, MQTT, BLE, Network Watchdog
Core 1: Servo Control, I2C, Health, Analytics, Heart Rate
Available interfaces: BLE, WiFi/MQTT
Real-time analytics: ENABLED
Biometric monitoring: ENABLED
```

### Test 2: Mobile App Connection
1. **Open the mobile app** on your Android device
2. **Forget any previous ESP32 pairings** in Android Bluetooth settings
3. **Tap "📡 Scan for ESP32"** in the app
4. **Wait for connection** (should show "Connected ✅")
5. **Test with "🔄 Run Servo Test"** button

## Step 4: Demo the System (5 minutes)

1. **Connect** mobile app to ESP32 using scan button
2. **Test "🔄 Run Servo Test"**: Single servo test movement
3. **Test "📋 Sequential Movement"**: Servos move one at a time
4. **Test "⚡ Simultaneous Movement"**: All servos move together
5. **Test "⏹️ Stop All Servos"**: Return to idle position
6. **Test Biometric Monitoring**: Place finger on heart rate sensor and check web dashboard at `http://your-server-ip:3000`

## Understanding the Control Modes

### 🔄 Run Servo Test
- Tests individual servo (servo 3)
- 3 cycles of 90° movements
- Verifies basic functionality
- Good for initial testing

### 📋 Sequential Movement
- Servos move one at a time
- 3 complete cycles per servo
- Reduces peak current draw
- Mimics finger movement patterns

### ⚡ Simultaneous Movement
- All servos move together
- 3 complete cycles
- Maximum power requirement
- Coordinated hand motion

### ⏹️ Stop All Servos
- Returns all servos to 0° position
- Low power consumption
- Safe idle state

## Troubleshooting

### Common Issues

#### "Device not found" in mobile app
- Check ESP32 is powered and running
- Verify Bluetooth is enabled on mobile device
- Try restarting ESP32

#### Servos not moving
- Check wiring connections
- Verify power supply (USB may be insufficient)
- Check serial monitor for error messages

#### Heart rate sensor not working
- Verify I2C connections (SCL: GPIO 21, SDA: GPIO 18)
- Check for "Heart rate sensor detected at address 0x57" in serial monitor
- Ensure sensor is powered with 3.3V (not 5V)
- Place finger firmly on sensor surface

#### ESP32 keeps resetting
- Power supply insufficient
- Try external 5V power supply
- Reduce servo load

#### Mobile app won't connect
- Ensure ESP32 is advertising (check serial monitor)
- Try closing and reopening the app
- Check device permissions for Bluetooth

### Getting Help

1. **Check serial monitor** for ESP32 status messages
2. **Review wiring** against the hardware setup guide
3. **Test with Python script** to isolate BLE issues
4. **Check power supply** if servos are erratic

## Next Steps

### For Development
- Modify servo movement patterns in `src/main.cpp`
- Add new states or movement sequences
- Implement position feedback
- Add safety features

### For Production
- Use external power supply
- Add proper enclosure
- Implement emergency stop
- Add position sensors
- Consider force feedback

### For Rehabilitation Use
- Consult medical professionals
- Implement safety interlocks
- Add force limiting
- Ensure regulatory compliance
- Conduct proper testing

## File Structure

```
ESP_Rehab_Hand_Exoskeleton/
├── src/
│   └── main.cpp              # ESP32 firmware
├── mobile-app/
│   ├── App.js               # React Native mobile app
│   ├── package.json         # Dependencies
│   └── app.json            # Expo configuration
├── platformio.ini           # PlatformIO configuration
├── test_servo.py           # Python BLE test script
├── setup.sh               # Automated setup script
├── README.md              # Main documentation
├── HARDWARE_SETUP.md      # Hardware wiring guide
└── GETTING_STARTED.md     # This file
```

## Key Features Implemented

### ESP32 Firmware
- ✅ BLE server with custom service
- ✅ FreeRTOS task management (8 tasks across dual cores)
- ✅ Power-optimized servo control
- ✅ Interruptible movement sequences
- ✅ Real-time status feedback
- ✅ Heart rate and SpO2 monitoring
- ✅ I2C sensor communication
- ✅ Real-time biometric data publishing

### Mobile App
- ✅ BLE device scanning and connection
- ✅ Real-time command sending
- ✅ Status monitoring
- ✅ User-friendly interface
- ✅ Connection state management

### Testing Tools
- ✅ Python BLE test script
- ✅ Serial monitor integration
- ✅ Automated setup script
- ✅ Comprehensive documentation

## Performance Characteristics

- **BLE Range**: ~10 meters (line of sight)
- **Command Response**: <100ms
- **Movement Precision**: ±1° (servo dependent)
- **Power Consumption**: ~1.5A peak (all servos)
- **Operating Voltage**: 5V ±10%

## Safety Notes

⚠️ **Important Safety Information**:
- This is a demonstration project
- Not certified for medical use
- Implement proper safety measures for human interaction
- Use appropriate power supplies
- Follow electrical safety guidelines

## Demo Script

For presentations or demonstrations:

1. **Introduction** (1 min)
   - Show hardware setup
   - Explain rehabilitation application

2. **Connection Demo** (2 min)
   - Start mobile app
   - Show BLE scanning and connection
   - Display real-time status

3. **Movement Demo** (3 min)
   - Demonstrate each state
   - Show interruption capability
   - Highlight power optimization

4. **Technical Overview** (2 min)
   - Explain FreeRTOS implementation
   - Discuss BLE communication
   - Show code structure

5. **Q&A** (2 min)
   - Answer technical questions
   - Discuss future enhancements

Total demo time: ~10 minutes

## Success Criteria

Your setup is successful when:
- ✅ ESP32 boots and advertises BLE service
- ✅ Heart rate sensor is detected at I2C address 0x57
- ✅ Mobile app can discover and connect to ESP32
- ✅ All three states work correctly
- ✅ Servos move smoothly without binding
- ✅ Commands are acknowledged in real-time
- ✅ System can be interrupted and state changed
- ✅ Heart rate data appears in web dashboard when finger is placed on sensor
- ✅ Biometric data updates in real-time (every 2-3 seconds)

Congratulations! You now have a working ESP32 servo controller for rehabilitation hand exoskeleton demonstrations.
