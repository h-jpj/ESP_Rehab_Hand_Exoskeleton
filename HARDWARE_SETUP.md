# Hardware Setup Guide

## Components Required

### ESP32 Development Board
- ESP32-WROOM-32 or ESP32-DevKitC
- USB-C or Micro-USB cable for programming and power
- Built-in WiFi and Bluetooth capabilities

### Servo Motors
- 3x SG90 Micro Servos (or similar 5V servos)
- Operating voltage: 4.8V - 6V
- Stall current: ~1.5A per servo
- Operating current: ~200mA per servo

### Heart Rate Sensor (Biometric Monitoring)
- GY-MAX30102 Heart Rate Pulse Oximetry Sensor
- Operating voltage: 3.3V
- Communication: I2C protocol
- Features: Heart rate detection, SpO2 measurement, finger detection

### Power Considerations
- **Demo Setup**: USB power through ESP32 Vin pin
- **Production Setup**: External 5V power supply (recommended 3A+)

## Wiring Connections

### Servo Connections
```
ESP32 Pin    | Servo Wire  | Description
-------------|-------------|------------------
GPIO 19      | Signal      | Servo 1 Control
GPIO 22      | Signal      | Servo 2 Control
GPIO 23      | Signal      | Servo 3 Control
Vin (5V)     | Red/Power   | All Servo Power
GND          | Brown/Black | All Servo Ground
```

### Heart Rate Sensor Connections (GY-MAX30102)
```
ESP32 Pin    | Sensor Pin  | Description
-------------|-------------|------------------
GPIO 21      | SCL         | I2C Clock Line
GPIO 18      | SDA         | I2C Data Line
GPIO 4       | INT         | Interrupt Pin (optional)
3.3V         | VCC         | Power Supply
GND          | GND         | Ground
```

## Circuit Diagram

![Circuit Diagram](circuit_diagram.svg)

*Professional circuit diagram showing all connections, power rails, and component specifications*

## Physical Wiring Diagram

```
ESP32                    Servo Motors              Heart Rate Sensor
┌─────────────┐         ┌──────────┐              ┌─────────────┐
│             │         │  Servo 1 │              │ GY-MAX30102 │
│        GPIO19├─────────┤Signal    │              │             │
│        GPIO21├─────────┼──────────┼──────────────┤SCL          │
│        GPIO18├─────────┼──────────┼──────────────┤SDA          │
│         GPIO4├─────────┼──────────┼──────────────┤INT          │
│             │         │Power ────┤              │VCC ─────────┤
│             │         │GND   ────┤              │GND ─────────┤
│             │         └──────────┘              └─────────────┘
│             │
│        GPIO22├─────────┐ ┌──────────┐
│             │         └─┤  Servo 2 │
│             │           │Signal    │
│             │           │Power ────┤
│             │           │GND   ────┤
│             │           └──────────┘
│             │
│        GPIO23├─────────┐ ┌──────────┐
│             │         └─┤  Servo 3 │
│             │           │Signal    │
│             │           │Power ────┤
│             │           │GND   ────┤
│             │           └──────────┘
│             │
│           Vin├───────────┴─────────── +5V Power Rail (Servos)
│          3.3V├─────────────────────── +3.3V Power Rail (Sensor)
│           GND├───────────┬─────────── Ground Rail (All Components)
│             │           │
│        USB-C │           │ (Connect all power and ground
│     (Power & │           │  wires to appropriate rails)
│   Programming)│           │
└─────────────┘
```

## Step-by-Step Assembly

### 1. Prepare the ESP32
- Ensure ESP32 is not powered during wiring
- Identify the GPIO pins: 19, 22, 23 (servos), 21, 18, 4 (heart rate sensor)
- Locate Vin, 3.3V, and GND pins

### 2. Prepare Servo Wires
- **Signal wires** (usually orange/yellow): Connect individually
- **Power wires** (usually red): Connect together to Vin
- **Ground wires** (usually brown/black): Connect together to GND

### 3. Prepare Heart Rate Sensor
- **SCL wire**: Connect to GPIO 21
- **SDA wire**: Connect to GPIO 18
- **INT wire**: Connect to GPIO 4 (optional but recommended)
- **VCC wire**: Connect to 3.3V pin
- **GND wire**: Connect to GND pin

### 4. Make Connections
1. Connect Servo 1 signal wire to GPIO 19
2. Connect Servo 2 signal wire to GPIO 22
3. Connect Servo 3 signal wire to GPIO 23
4. Connect all servo power wires to ESP32 Vin pin
5. Connect all servo ground wires to ESP32 GND pin
6. Connect heart rate sensor SCL to GPIO 21
7. Connect heart rate sensor SDA to GPIO 18
8. Connect heart rate sensor INT to GPIO 4
9. Connect heart rate sensor VCC to 3.3V pin
10. Connect heart rate sensor GND to ESP32 GND pin

### 5. Verify Connections
- Double-check all signal wire connections (servos and sensor)
- Ensure power and ground are properly connected
- Verify I2C connections (SCL, SDA) are correct
- Check for any short circuits
- Confirm 3.3V and 5V power rails are not crossed

## Power Supply Notes

### USB Power (Demo Setup)
- **Pros**: Simple, no external components needed
- **Cons**: Limited current, may cause brownouts
- **Suitable for**: Short demonstrations, light loads
- **Max current**: ~500mA from USB, shared between ESP32 and servos

### External Power (Production Setup)
- **Recommended**: 5V 3A power supply
- **Connection**: Power supply positive to Vin, negative to GND
- **Benefits**: Stable operation, no current limitations
- **Required for**: Continuous operation, heavy loads

## Safety Considerations

### Electrical Safety
- Never connect/disconnect while powered
- Check polarity before applying power
- Use appropriate wire gauge for current
- Avoid short circuits

### Mechanical Safety
- Secure servo mounting to prevent damage
- Ensure servo arms don't interfere with each other
- Consider servo stall conditions
- Implement emergency stop if needed

## Troubleshooting

### Servo Not Moving
1. Check signal wire connection
2. Verify power supply voltage (should be 4.8-6V)
3. Check servo for mechanical binding
4. Verify code is uploading correctly

### ESP32 Resets/Brownouts
1. **Cause**: Insufficient power supply
2. **Solution**: Use external 5V power supply
3. **Temporary fix**: Reduce servo load or movement speed

### Erratic Servo Movement
1. Check for loose connections
2. Verify ground connections
3. Check for electromagnetic interference
4. Ensure adequate power supply current

### BLE Connection Issues
1. Verify ESP32 is advertising (check serial monitor)
2. Ensure mobile device Bluetooth is enabled
3. Check for interference from other devices
4. Try restarting both ESP32 and mobile app

## Testing Procedure

### 1. Initial Power Test
- Connect USB cable to ESP32
- Check that ESP32 power LED illuminates
- Verify no smoke or unusual heating

### 2. Servo Position Test
- Upload firmware to ESP32
- Observe servos move to initial position (0°)
- Check serial monitor for startup messages

### 3. Heart Rate Sensor Test
- Check serial monitor for I2C device detection
- Look for "Heart rate sensor detected at address 0x57"
- Place finger on sensor and verify readings appear
- Confirm finger detection and signal quality indicators

### 4. BLE Communication Test
- Run mobile app or Python test script
- Verify ESP32 appears in device scan
- Test connection and command sending

### 5. Movement Test
- Test each state (0, 1, 2) individually
- Verify smooth servo operation
- Check for any mechanical binding

### 6. Biometric Integration Test
- Start a therapy session via mobile app
- Place finger on heart rate sensor
- Verify real-time biometric data appears in web dashboard
- Check that heart rate and SpO2 values update every few seconds

## Maintenance

### Regular Checks
- Inspect wire connections for looseness
- Check servo operation for smoothness
- Monitor power supply stability
- Clean contacts if necessary

### Servo Care
- Avoid forcing servo beyond limits
- Keep servo gears clean and lubricated
- Replace servos if they become noisy or erratic
- Store in dry environment when not in use

## Upgrade Path

### For Production Use
1. **External Power Supply**: 5V 3A regulated supply
2. **Power Distribution**: Dedicated servo power bus
3. **Protection**: Fuses and reverse polarity protection
4. **Enclosure**: Proper housing for electronics
5. **Connectors**: Reliable servo connectors
6. **Emergency Stop**: Hardware emergency stop button
