# Project Structure

## Directory Layout

```
ESP_Rehab_Hand_Exoskeleton/
├── README.md                    # Main project documentation
├── GETTING_STARTED.md          # Setup instructions
├── HARDWARE_SETUP.md           # Hardware wiring guide
├── PROJECT_STRUCTURE.md        # This file
├── .gitignore                  # Git ignore rules
├── platformio.ini              # PlatformIO configuration
├── src/                        # ESP32 firmware source
│   └── main.cpp                # Main ESP32 application
└── servo-controller/           # React Native mobile app
    ├── App.tsx                 # Main app component
    ├── package.json            # Node.js dependencies
    ├── app.json                # Expo configuration
    ├── tsconfig.json           # TypeScript configuration
    ├── android/                # Android build files
    └── assets/                 # App assets (icons, etc.)
```

## Key Files

### ESP32 Firmware (`src/main.cpp`)
- **Purpose**: Main ESP32 application with BLE server
- **Features**:
  - NimBLE server implementation
  - Servo control with FreeRTOS tasks
  - Sequential and simultaneous movement patterns
  - Power-optimized servo management

### Mobile App (`servo-controller/App.tsx`)
- **Purpose**: React Native app for BLE control
- **Features**:
  - BLE scanning and connection
  - Servo control interface
  - Real-time status updates
  - Debug information display

### Configuration Files
- **`platformio.ini`**: ESP32 build configuration and dependencies
- **`servo-controller/package.json`**: Mobile app dependencies
- **`servo-controller/app.json`**: Expo app configuration

## Dependencies

### ESP32 (PlatformIO)
- **Framework**: Arduino
- **Platform**: Espressif 32
- **Libraries**:
  - ESP32Servo (servo control)
  - NimBLE-Arduino (Bluetooth Low Energy)

### Mobile App (React Native)
- **Framework**: Expo
- **Key Dependencies**:
  - react-native-ble-plx (BLE communication)
  - expo-status-bar (status bar control)
  - TypeScript support

## Build Outputs

### ESP32
- **Binary**: `.pio/build/esp32dev/firmware.bin`
- **Upload**: Direct to ESP32 via USB

### Mobile App
- **Development**: Expo development server
- **Production**: Android APK via `expo run:android`

## Development Workflow

1. **ESP32 Development**:
   ```bash
   pio run --target upload    # Build and upload
   pio device monitor         # Monitor serial output
   ```

2. **Mobile App Development**:
   ```bash
   cd servo-controller
   npm install                # Install dependencies
   npx expo run:android       # Build and run
   ```

## Communication Protocol

### BLE Service
- **Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`

### Commands
- `"TEST"` → Servo test sequence
- `"0"` → Stop/idle state
- `"1"` → Sequential movement
- `"2"` → Simultaneous movement

### Responses
- `STATE_CHANGED:X` → Command acknowledged
- `TEST_COMPLETE` → Test finished
