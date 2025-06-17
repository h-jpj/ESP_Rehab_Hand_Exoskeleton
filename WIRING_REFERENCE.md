# Quick Wiring Reference

## ESP32 Rehabilitation Hand Exoskeleton - Connection Guide

### 🔌 Complete Connection Table

| Component | Pin/Wire | ESP32 Pin | Description | Wire Color |
|-----------|----------|-----------|-------------|------------|
| **Servo 1** | Signal | GPIO 19 | PWM Control | Orange/Yellow |
| **Servo 1** | Power | Vin (5V) | Motor Power | Red |
| **Servo 1** | Ground | GND | Common Ground | Brown/Black |
| **Servo 2** | Signal | GPIO 22 | PWM Control | Orange/Yellow |
| **Servo 2** | Power | Vin (5V) | Motor Power | Red |
| **Servo 2** | Ground | GND | Common Ground | Brown/Black |
| **Servo 3** | Signal | GPIO 23 | PWM Control | Orange/Yellow |
| **Servo 3** | Power | Vin (5V) | Motor Power | Red |
| **Servo 3** | Ground | GND | Common Ground | Brown/Black |
| **Heart Rate** | SCL | GPIO 21 | I2C Clock | Purple |
| **Heart Rate** | SDA | GPIO 18 | I2C Data | Purple |
| **Heart Rate** | INT | GPIO 4 | Interrupt (Optional) | Gray |
| **Heart Rate** | VCC | 3.3V | Sensor Power | Red |
| **Heart Rate** | GND | GND | Common Ground | Black |

### ⚡ Power Distribution

```
USB-C (5V) ──┬── ESP32 Vin ──┬── Servo 1 Power (Red)
             │               ├── Servo 2 Power (Red)
             │               └── Servo 3 Power (Red)
             │
             └── ESP32 3.3V ──── Heart Rate VCC (Red)

ESP32 GND ──┬── Servo 1 Ground (Brown/Black)
            ├── Servo 2 Ground (Brown/Black)
            ├── Servo 3 Ground (Brown/Black)
            └── Heart Rate GND (Black)
```

### 🎯 GPIO Pin Summary

| GPIO Pin | Function | Component | Signal Type |
|----------|----------|-----------|-------------|
| GPIO 19 | Servo 1 Control | Servo Motor 1 | PWM Output |
| GPIO 22 | Servo 2 Control | Servo Motor 2 | PWM Output |
| GPIO 23 | Servo 3 Control | Servo Motor 3 | PWM Output |
| GPIO 21 | I2C Clock | Heart Rate Sensor | I2C SCL |
| GPIO 18 | I2C Data | Heart Rate Sensor | I2C SDA |
| GPIO 4 | Interrupt | Heart Rate Sensor | Digital Input |

### 🔧 Assembly Checklist

#### Before Powering On:
- [ ] All servo signal wires connected to correct GPIO pins
- [ ] All servo power wires connected to Vin (5V)
- [ ] All servo ground wires connected to GND
- [ ] Heart rate sensor SCL connected to GPIO 21
- [ ] Heart rate sensor SDA connected to GPIO 18
- [ ] Heart rate sensor INT connected to GPIO 4
- [ ] Heart rate sensor VCC connected to 3.3V (NOT 5V!)
- [ ] Heart rate sensor GND connected to GND
- [ ] No short circuits between power and ground
- [ ] All connections secure and properly insulated

#### Power-On Verification:
- [ ] ESP32 power LED illuminates
- [ ] No smoke or unusual heating
- [ ] Serial monitor shows system initialization
- [ ] Heart rate sensor detected at I2C address 0x57
- [ ] All servos move to home position (0°)
- [ ] BLE advertising starts successfully

### ⚠️ Critical Safety Notes

1. **Voltage Levels**:
   - Servos: 5V (from Vin pin)
   - Heart Rate Sensor: 3.3V (NOT 5V - will damage sensor!)

2. **Power Requirements**:
   - Demo: USB 3.0 port (sufficient for testing)
   - Production: External 5V/3A power supply recommended

3. **I2C Pull-ups**:
   - ESP32 has internal pull-ups enabled
   - External pull-ups not required for this setup

4. **Current Draw**:
   - Each servo: ~200mA normal, ~1.5A stall
   - Heart rate sensor: ~1.5mA
   - ESP32: ~240mA with WiFi/BLE active

### 🔍 Troubleshooting Quick Reference

| Problem | Check | Solution |
|---------|-------|----------|
| Servos not moving | GPIO connections | Verify GPIO 19, 22, 23 |
| ESP32 resets | Power supply | Use external 5V supply |
| Heart rate not detected | I2C wiring | Check GPIO 21 (SCL), 18 (SDA) |
| Sensor shows 0x00 | Power voltage | Ensure 3.3V, not 5V |
| Erratic servo movement | Ground connections | Check all GND connections |
| BLE not advertising | Serial monitor | Check for error messages |

### 📐 Physical Layout Tips

1. **Servo Placement**: Mount servos to minimize wire length
2. **Sensor Position**: Place heart rate sensor for easy finger access
3. **ESP32 Location**: Central position to minimize wire runs
4. **Power Distribution**: Use breadboard or PCB for clean connections
5. **Wire Management**: Use different colors for easy identification

### 🔄 Testing Sequence

1. **Power Test**: Connect USB, verify ESP32 LED
2. **Serial Test**: Check initialization messages
3. **Servo Test**: Verify all servos move to home position
4. **I2C Test**: Confirm heart rate sensor detection
5. **BLE Test**: Verify mobile app can connect
6. **Function Test**: Test all movement patterns
7. **Biometric Test**: Place finger on sensor, verify readings

---

**Remember**: This is a demonstration project. Implement proper safety measures for any human interaction!
