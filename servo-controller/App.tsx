import React, { useState, useEffect } from 'react';
import {
  StyleSheet,
  Text,
  View,
  TouchableOpacity,
  Alert,
  PermissionsAndroid,
  Platform,
} from 'react-native';
import { StatusBar } from 'expo-status-bar';
import { BleManager, Device, Characteristic } from 'react-native-ble-plx';

const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8';
const DEVICE_NAME = 'ESP32_Servo';

// Session management types
interface SessionState {
  isActive: boolean;
  sessionId: string | null;
  startTime: Date | null;
  duration: number;
  movementsCompleted: number;
  sessionType: 'unknown' | 'sequential' | 'simultaneous' | 'mixed' | 'test';
}

export default function App() {
  const [bleManager, setBleManager] = useState<BleManager | null>(null);
  const [device, setDevice] = useState<Device | null>(null);
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [characteristic, setCharacteristic] = useState<Characteristic | null>(null);
  const [initError, setInitError] = useState<string | null>(null);

  // Session management state
  const [sessionState, setSessionState] = useState<SessionState>({
    isActive: false,
    sessionId: null,
    startTime: null,
    duration: 0,
    movementsCompleted: 0,
    sessionType: 'unknown'
  });

  // Timer for session duration updates
  const [sessionTimer, setSessionTimer] = useState<NodeJS.Timeout | null>(null);

  // Session management functions
  const startSession = () => {
    const now = new Date();
    setSessionState({
      isActive: true,
      sessionId: `SES_${now.getTime()}`,
      startTime: now,
      duration: 0,
      movementsCompleted: 0,
      sessionType: 'unknown'
    });

    // Start timer to update duration every second
    const timer = setInterval(() => {
      setSessionState(prev => {
        if (prev.isActive && prev.startTime) {
          return {
            ...prev,
            duration: Math.floor((Date.now() - prev.startTime.getTime()) / 1000)
          };
        }
        return prev;
      });
    }, 1000);

    setSessionTimer(timer);
    console.log('Session started automatically on BLE connection');
  };

  const endSession = async () => {
    if (!sessionState.isActive) {
      Alert.alert('No Active Session', 'There is no active session to end.');
      return;
    }

    try {
      // Send END_SESSION command to ESP32
      await sendCommand('END_SESSION', 'Session ended successfully!');

      // Clear timer
      if (sessionTimer) {
        clearInterval(sessionTimer);
        setSessionTimer(null);
      }

      // Update session state
      setSessionState(prev => ({
        ...prev,
        isActive: false
      }));

      console.log('Session ended by user request');
    } catch (error) {
      console.error('Error ending session:', error);
      Alert.alert('Error', 'Failed to end session properly');
    }
  };

  const updateMovementCount = (command: string) => {
    if (sessionState.isActive) {
      setSessionState(prev => {
        const newCount = prev.movementsCompleted + 1;
        let newType = prev.sessionType;

        // Update session type based on commands
        if (command === '1') {
          newType = prev.sessionType === 'simultaneous' ? 'mixed' : 'sequential';
        } else if (command === '2') {
          newType = prev.sessionType === 'sequential' ? 'mixed' : 'simultaneous';
        } else if (command === 'TEST') {
          newType = prev.sessionType === 'unknown' ? 'test' : prev.sessionType;
        }

        return {
          ...prev,
          movementsCompleted: newCount,
          sessionType: newType
        };
      });
    }
  };

  // Request Bluetooth permissions for Android
  const requestBluetoothPermissions = async (): Promise<boolean> => {
    if (Platform.OS !== 'android') {
      return true; // iOS handles permissions differently
    }

    try {
      const permissions = [
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      ];

      const granted = await PermissionsAndroid.requestMultiple(permissions);

      const allGranted = Object.values(granted).every(
        permission => permission === PermissionsAndroid.RESULTS.GRANTED
      );

      if (allGranted) {
        console.log('All Bluetooth permissions granted');
        return true;
      } else {
        console.log('Some Bluetooth permissions denied, but continuing anyway...');
        // Continue anyway - the user can connect manually via Android settings
        return true;
      }
    } catch (error) {
      console.error('Permission request error:', error);
      // Continue anyway - the user can connect manually via Android settings
      return true;
    }
  };

  useEffect(() => {
    const initializeBluetooth = async () => {
      try {
        console.log('Requesting Bluetooth permissions...');
        await requestBluetoothPermissions();

        console.log('Initializing BLE Manager...');
        const manager = new BleManager();
        setBleManager(manager);

        // Check for already connected ESP32 devices
        setTimeout(async () => {
          try {
            const connectedDevices = await manager.connectedDevices([SERVICE_UUID]);
            if (connectedDevices.length > 0) {
              console.log('Found connected ESP32, setting up characteristic...');
              await connectToDevice(connectedDevices[0]);
            } else {
              console.log('No connected ESP32 found - connect manually via Android Bluetooth settings');
            }
          } catch (error) {
            console.log('No connected ESP32 found - connect manually via Android Bluetooth settings');
          }
        }, 2000);

        return () => {
          manager.destroy();
        };
      } catch (error: any) {
        console.error('BLE Manager initialization error:', error);
        setInitError(error.message || 'Failed to initialize Bluetooth');
      }
    };

    initializeBluetooth();
  }, []);

  const findConnectedESP32 = async (): Promise<void> => {
    if (!bleManager) {
      Alert.alert('Error', 'Please restart the app');
      return;
    }

    try {
      console.log('Looking for connected ESP32...');
      const connectedDevices = await bleManager.connectedDevices([SERVICE_UUID]);

      if (connectedDevices.length > 0) {
        const device = connectedDevices[0];
        console.log(`Found connected ESP32: ${device.name}`);
        await connectToDevice(device);
        Alert.alert('Success', 'Connected to ESP32!');
      } else {
        Alert.alert('ESP32 Not Found', 'Please connect to ESP32 via Android Bluetooth settings first.');
      }
    } catch (error: any) {
      console.error('Find ESP32 error:', error);
      Alert.alert('Error', 'Failed to find ESP32. Make sure it\'s paired in Bluetooth settings.');
    }
  };

  const connectToDevice = async (scannedDevice: Device): Promise<void> => {
    try {
      console.log(`Connecting to device: ${scannedDevice.name} (${scannedDevice.id})`);

      // Check if already connected
      const isAlreadyConnected = await scannedDevice.isConnected();
      let connectedDevice = scannedDevice;

      if (!isAlreadyConnected) {
        console.log('Device not connected, connecting...');
        connectedDevice = await scannedDevice.connect();
      } else {
        console.log('Device already connected');
      }

      setDevice(connectedDevice);

      console.log('Connection established, waiting for stabilization...');
      await new Promise(resolve => setTimeout(resolve, 1000)); // Wait 1 second

      console.log('Discovering services...');
      await connectedDevice.discoverAllServicesAndCharacteristics();

      const services = await connectedDevice.services();
      console.log(`Found ${services.length} services`);

      const targetService = services.find(service => service.uuid.toLowerCase() === SERVICE_UUID.toLowerCase());

      if (targetService) {
        console.log('Found target service');
        const characteristics = await targetService.characteristics();
        console.log(`Found ${characteristics.length} characteristics`);

        const targetCharacteristic = characteristics.find(
          char => char.uuid.toLowerCase() === CHARACTERISTIC_UUID.toLowerCase()
        );

        if (targetCharacteristic) {
          setCharacteristic(targetCharacteristic);
          setIsConnected(true);
          console.log('Connected successfully!');

          // Auto-start session on successful connection
          startSession();

          // Monitor disconnection
          connectedDevice.onDisconnected((error, device) => {
            console.log('Device disconnected:', device?.name);
            setIsConnected(false);
            setCharacteristic(null);
            setDevice(null);

            // End session on disconnection
            if (sessionState.isActive) {
              if (sessionTimer) {
                clearInterval(sessionTimer);
                setSessionTimer(null);
              }
              setSessionState(prev => ({ ...prev, isActive: false }));
              console.log('Session ended due to disconnection');
            }

            if (error) {
              console.error('Disconnection error:', error);
            }
          });
        } else {
          console.error('Characteristic not found');
          Alert.alert('Error', 'Characteristic not found');
        }
      } else {
        console.error('Service not found');
        Alert.alert('Error', 'Service not found');
      }
    } catch (error: any) {
      console.error('Connection error:', error);
      Alert.alert('Connection Failed', error.message || 'Unknown error');
    }
  };

  const disconnect = async (): Promise<void> => {
    if (device) {
      try {
        await device.cancelConnection();
        setDevice(null);
        setCharacteristic(null);
        setIsConnected(false);
        Alert.alert('Disconnected', 'ESP32 disconnected');
      } catch (error: any) {
        console.error('Disconnect error:', error);
        Alert.alert('Disconnect Error', error.message || 'Unknown error');
      }
    }
  };

  const sendCommand = async (command: string, description: string): Promise<void> => {
    if (!bleManager) {
      Alert.alert('Error', 'Bluetooth not initialized. Please restart the app.');
      return;
    }

    try {
      // Try to find connected ESP32 if we don't have a characteristic
      if (!characteristic) {
        console.log('No characteristic found, scanning for all connected devices...');

        // Try to get all connected devices (not just by service UUID)
        const allDevices = await bleManager.connectedDevices([]);
        console.log(`Found ${allDevices.length} connected devices`);

        // Look for ESP32 by name
        const esp32Device = allDevices.find(device =>
          device.name && (device.name.includes('ESP32') || device.name === 'ESP32_Servo')
        );

        if (esp32Device) {
          console.log(`Found ESP32 device: ${esp32Device.name}`);
          await connectToDevice(esp32Device);
        } else {
          // If still not found, try with service UUID
          const connectedDevices = await bleManager.connectedDevices([SERVICE_UUID]);
          if (connectedDevices.length > 0) {
            console.log('Found device with service UUID');
            await connectToDevice(connectedDevices[0]);
          } else {
            Alert.alert('Error', 'ESP32 not found. Make sure it\'s connected and try again.');
            return;
          }
        }
      }

      if (characteristic) {
        // Convert string to base64 without Buffer
        const base64Command = btoa(command);
        console.log(`Sending command: "${command}" as base64: "${base64Command}"`);
        await characteristic.writeWithResponse(base64Command);
        console.log(`Command sent successfully: ${command}`);

        // Track movement commands in session
        if (['1', '2', 'TEST'].includes(command)) {
          updateMovementCount(command);
        }

        Alert.alert('Success', description);
      } else {
        Alert.alert('Error', 'Could not establish connection to ESP32');
      }
    } catch (error: any) {
      console.error('Send error:', error);
      Alert.alert('Error', `Failed to send command: ${error.message}`);
    }
  };

  const [isScanning, setIsScanning] = useState<boolean>(false);

  const scanAndConnect = async () => {
    if (!bleManager || isScanning) {
      console.log('Scan blocked: bleManager exists?', !!bleManager, 'isScanning?', isScanning);
      return;
    }

    setIsScanning(true);
    console.log('=== STARTING BLE SCAN ===');

    let deviceCount = 0;
    let esp32Found = false;

    try {
      bleManager.startDeviceScan(null, null, (error, scannedDevice) => {
        if (error) {
          console.error('Scan callback error:', error);
          setIsScanning(false);
          Alert.alert('Scan Error', error.message);
          return;
        }

        if (scannedDevice) {
          deviceCount++;
          console.log(`Device ${deviceCount}: ${scannedDevice.name || 'Unknown'} (${scannedDevice.id})`);
          console.log(`  RSSI: ${scannedDevice.rssi}, Connectable: ${scannedDevice.isConnectable}`);

          if (scannedDevice.name && scannedDevice.name.includes('ESP32')) {
            console.log(`*** ESP32 FOUND: ${scannedDevice.name} ***`);
            esp32Found = true;
            bleManager.stopDeviceScan();
            setIsScanning(false);
            connectToDevice(scannedDevice);
          }
        }
      });

      console.log('Scan started successfully, waiting for devices...');

      // Stop scanning after 15 seconds
      setTimeout(() => {
        console.log(`Scan timeout reached. Found ${deviceCount} devices total, ESP32 found: ${esp32Found}`);
        if (isScanning) {
          bleManager.stopDeviceScan();
          setIsScanning(false);
          Alert.alert('Scan Complete', `Found ${deviceCount} devices, but no ESP32. Make sure ESP32 is powered on and advertising.`);
        }
      }, 15000);

    } catch (error: any) {
      console.error('Scan start error:', error);
      setIsScanning(false);
      Alert.alert('Error', `Failed to start scan: ${error.message}`);
    }
  };

  const debugConnection = async () => {
    if (!bleManager) {
      Alert.alert('Debug', 'BLE Manager not initialized');
      return;
    }

    try {
      console.log('=== DETAILED DEBUG INFO ===');

      // Check BLE manager state
      const state = await bleManager.state();
      console.log(`BLE Manager State: ${state}`);

      // Check all connected devices
      const allDevices = await bleManager.connectedDevices([]);
      console.log(`All connected devices: ${allDevices.length}`);
      allDevices.forEach((device, index) => {
        console.log(`Device ${index + 1}:`);
        console.log(`  Name: ${device.name || 'Unknown'}`);
        console.log(`  ID: ${device.id}`);
        console.log(`  RSSI: ${device.rssi || 'Unknown'}`);
      });

      // Check devices with our service
      const serviceDevices = await bleManager.connectedDevices([SERVICE_UUID]);
      console.log(`Devices with our service UUID (${SERVICE_UUID}): ${serviceDevices.length}`);
      serviceDevices.forEach((device, index) => {
        console.log(`Service Device ${index + 1}:`);
        console.log(`  Name: ${device.name || 'Unknown'}`);
        console.log(`  ID: ${device.id}`);
      });

      // Check current app state
      console.log(`App State:`);
      console.log(`  isConnected: ${isConnected}`);
      console.log(`  characteristic: ${characteristic ? 'Set' : 'Not Set'}`);
      console.log(`  device: ${device ? device.name || 'Unknown' : 'Not Set'}`);

      // Check permissions
      let permissionInfo = 'Unknown';
      if (Platform.OS === 'android') {
        try {
          const bluetoothScan = await PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN);
          const bluetoothConnect = await PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT);
          const location = await PermissionsAndroid.check(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION);
          permissionInfo = `Scan:${bluetoothScan}, Connect:${bluetoothConnect}, Location:${location}`;
        } catch (e) {
          permissionInfo = 'Permission check failed';
        }
      }
      console.log(`Permissions: ${permissionInfo}`);

      const debugMessage = `BLE State: ${state}
All Devices: ${allDevices.length}
Service Devices: ${serviceDevices.length}
App Connected: ${isConnected}
Characteristic: ${characteristic ? 'Yes' : 'No'}
Permissions: ${permissionInfo}

Check console for full details`;

      Alert.alert('Debug Info', debugMessage);
    } catch (error: any) {
      console.error('Debug error:', error);
      Alert.alert('Debug Error', `${error.message}\n\nCheck console for details`);
    }
  };

  const stopScan = () => {
    if (bleManager && isScanning) {
      console.log('Manually stopping scan...');
      bleManager.stopDeviceScan();
      setIsScanning(false);
      Alert.alert('Scan Stopped', 'BLE scan stopped manually');
    }
  };

  const runServoTest = () => sendCommand("TEST", "Servo test started! (3x 90° movements)");
  const stopServos = () => sendCommand("0", "Servos stopped (idle state)");
  const sequentialMovement = () => sendCommand("1", "Sequential movement started! (one servo at a time)");
  const simultaneousMovement = () => sendCommand("2", "Simultaneous movement started! (all servos together)");

  return (
    <View style={styles.container}>
      <StatusBar style="auto" />

      <Text style={styles.title}>ESP32 Servo Control Panel</Text>

      {initError ? (
        <Text style={[styles.status, styles.errorText]}>
          Error: {initError}
        </Text>
      ) : (
        <>
          <Text style={styles.status}>
            ESP32 Status: {isConnected ? 'Connected ✅' : 'Not Connected ❌'}
          </Text>

          {/* Session Status Display */}
          {isConnected && (
            <View style={styles.sessionContainer}>
              <Text style={styles.sessionTitle}>Therapy Session</Text>
              {sessionState.isActive ? (
                <>
                  <Text style={styles.sessionStatus}>🟢 Session Active</Text>
                  <Text style={styles.sessionInfo}>
                    Duration: {Math.floor(sessionState.duration / 60)}:{(sessionState.duration % 60).toString().padStart(2, '0')}
                  </Text>
                  <Text style={styles.sessionInfo}>
                    Movements: {sessionState.movementsCompleted}
                  </Text>
                  <Text style={styles.sessionInfo}>
                    Type: {sessionState.sessionType.charAt(0).toUpperCase() + sessionState.sessionType.slice(1)}
                  </Text>
                </>
              ) : (
                <Text style={styles.sessionStatus}>⚪ No Active Session</Text>
              )}
            </View>
          )}
        </>
      )}

      {!isConnected && !initError && (
        <View style={styles.instructionsContainer}>
          <Text style={styles.instructionsTitle}>Connection Steps:</Text>
          <Text style={styles.instructionText}>
            1. If ESP32 was paired before, go to Android Bluetooth settings and "Forget" the ESP32_Servo device{'\n'}
            2. Make sure ESP32 is powered on and advertising{'\n'}
            3. Tap "📡 Scan for ESP32" below{'\n'}
            4. Connection should work automatically
          </Text>
        </View>
      )}

      {!initError && (
        <View style={styles.controlsContainer}>
          {!isConnected && (
            <>
              <TouchableOpacity
                style={[styles.button, styles.scanButton, isScanning && styles.disabledButton]}
                onPress={scanAndConnect}
                disabled={isScanning}
              >
                <Text style={styles.buttonText}>
                  {isScanning ? '🔄 Scanning...' : '📡 Scan for ESP32'}
                </Text>
              </TouchableOpacity>

              {isScanning && (
                <TouchableOpacity
                  style={[styles.button, styles.stopButton]}
                  onPress={stopScan}
                >
                  <Text style={styles.buttonText}>⏹️ Stop Scan</Text>
                </TouchableOpacity>
              )}
            </>
          )}

          {isConnected && (
            <>
              <Text style={styles.controlsTitle}>Servo Controls</Text>

              <TouchableOpacity style={[styles.button, styles.testButton]} onPress={runServoTest}>
                <Text style={styles.buttonText}>🔄 Run Servo Test</Text>
              </TouchableOpacity>

              <TouchableOpacity style={[styles.button, styles.sequentialButton]} onPress={sequentialMovement}>
                <Text style={styles.buttonText}>📋 Sequential Movement</Text>
              </TouchableOpacity>

              <TouchableOpacity style={[styles.button, styles.staggeredButton]} onPress={simultaneousMovement}>
                <Text style={styles.buttonText}>⚡ Simultaneous Movement</Text>
              </TouchableOpacity>

              <TouchableOpacity style={[styles.button, styles.stopButton]} onPress={stopServos}>
                <Text style={styles.buttonText}>⏹️ Stop All Servos</Text>
              </TouchableOpacity>

              {/* End Session Button */}
              {sessionState.isActive && (
                <TouchableOpacity
                  style={[styles.button, styles.endSessionButton]}
                  onPress={endSession}
                >
                  <Text style={styles.buttonText}>🛑 End Session</Text>
                </TouchableOpacity>
              )}
            </>
          )}

          <TouchableOpacity style={[styles.button, styles.debugButton]} onPress={debugConnection}>
            <Text style={styles.buttonText}>🔍 Debug Info</Text>
          </TouchableOpacity>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
    color: '#333',
  },
  status: {
    fontSize: 18,
    marginBottom: 30,
    color: '#666',
  },
  errorText: {
    color: '#FF3B30',
  },
  button: {
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 10,
    marginBottom: 15,
    minWidth: 200,
    alignItems: 'center',
  },
  connectButton: {
    backgroundColor: '#34C759',
  },
  controlsContainer: {
    alignItems: 'center',
    width: '100%',
  },
  instructionsContainer: {
    backgroundColor: '#F2F2F7',
    padding: 15,
    borderRadius: 10,
    marginBottom: 20,
    marginHorizontal: 20,
  },
  instructionsTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 10,
  },
  controlsTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 20,
    color: '#333',
  },
  scanButton: {
    backgroundColor: '#34C759',
    marginBottom: 15,
  },
  debugButton: {
    backgroundColor: '#5856D6',
    marginBottom: 10,
  },
  testButton: {
    backgroundColor: '#FF2D92',
    marginBottom: 20,
  },
  stopButton: {
    backgroundColor: '#8E8E93',
  },
  sequentialButton: {
    backgroundColor: '#007AFF',
  },
  staggeredButton: {
    backgroundColor: '#FF9500',
  },
  disconnectButton: {
    backgroundColor: '#FF3B30',
  },
  disabledButton: {
    backgroundColor: '#ccc',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  instructionText: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginTop: 30,
    paddingHorizontal: 20,
    lineHeight: 24,
  },
  // Session management styles
  sessionContainer: {
    backgroundColor: '#E8F4FD',
    padding: 15,
    borderRadius: 10,
    marginBottom: 20,
    marginHorizontal: 20,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#007AFF',
  },
  sessionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#007AFF',
    marginBottom: 10,
  },
  sessionStatus: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 8,
  },
  sessionInfo: {
    fontSize: 14,
    color: '#666',
    marginBottom: 4,
  },
  endSessionButton: {
    backgroundColor: '#FF3B30',
    marginTop: 10,
  },
});
