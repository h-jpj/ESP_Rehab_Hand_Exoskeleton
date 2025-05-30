#!/usr/bin/env node

const mqtt = require('mqtt');
const mysql = require('mysql2/promise');
const io = require('socket.io-client');

// Configuration
const MQTT_CONFIG = {
    host: process.env.MQTT_HOST || 'mosquitto',
    port: 1883,
    username: process.env.MQTT_USER || 'your_username',
    password: process.env.MQTT_PASSWORD || 'your_password'
};

const DB_CONFIG = {
    host: process.env.DB_HOST || 'mariadb',
    port: process.env.DB_PORT || 3306,
    user: process.env.DB_USER || 'your_db_username',
    password: process.env.DB_PASS || 'your_db_password',
    database: process.env.DB_NAME || 'rehab_exoskeleton'
};

// MQTT Topics to subscribe to
const TOPICS = [
    'rehab_exo/ESP32_001/movement/command',
    'rehab_exo/ESP32_001/system/status',
    'rehab_exo/ESP32_001/connection/wifi',
    'rehab_exo/ESP32_001/connection/ble',
    'rehab_exo/ESP32_001/session/start',
    'rehab_exo/ESP32_001/session/end',
    'rehab_exo/ESP32_001/session/progress'
];

let dbPool;
let mqttClient;
let socketClient;

// Initialize database connection
async function initDatabase() {
    try {
        dbPool = mysql.createPool(DB_CONFIG);
        console.log('✅ Database connection pool created');

        // Test connection
        const connection = await dbPool.getConnection();
        console.log('✅ Database connection tested successfully');
        connection.release();
    } catch (error) {
        console.error('❌ Database connection failed:', error.message);
        process.exit(1);
    }
}

// Initialize WebSocket connection to webapp
function initWebSocket() {
    socketClient = io('http://rehab_webapp:3000');

    socketClient.on('connect', () => {
        console.log('✅ WebSocket connected to webapp');
    });

    socketClient.on('disconnect', () => {
        console.log('📡 WebSocket disconnected from webapp');
    });

    socketClient.on('error', (error) => {
        console.error('❌ WebSocket error:', error);
    });
}

// Initialize MQTT connection
function initMQTT() {
    const clientId = `mqtt_bridge_${Math.random().toString(16).substr(2, 8)}`;

    mqttClient = mqtt.connect(`mqtt://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}`, {
        clientId: clientId,
        username: MQTT_CONFIG.username,
        password: MQTT_CONFIG.password,
        clean: true,
        reconnectPeriod: 5000
    });

    mqttClient.on('connect', () => {
        console.log('✅ MQTT connected');

        // Subscribe to all topics
        TOPICS.forEach(topic => {
            mqttClient.subscribe(topic, (err) => {
                if (err) {
                    console.error(`❌ Failed to subscribe to ${topic}:`, err);
                } else {
                    console.log(`📡 Subscribed to: ${topic}`);
                }
            });
        });
    });

    mqttClient.on('message', handleMQTTMessage);

    mqttClient.on('error', (error) => {
        console.error('❌ MQTT error:', error);
    });

    mqttClient.on('disconnect', () => {
        console.log('📡 MQTT disconnected');
    });
}

// Handle incoming MQTT messages
async function handleMQTTMessage(topic, message) {
    try {
        const data = JSON.parse(message.toString());
        console.log(`📨 Received: ${topic}`);
        console.log(`📄 Data:`, JSON.stringify(data, null, 2));

        await processMessage(topic, data);

    } catch (error) {
        console.error('❌ Error processing message:', error);
        console.error('📄 Raw message:', message.toString());
    }
}

// Process and store message in database
async function processMessage(topic, data) {
    const { device_id, timestamp, event_type } = data;

    if (event_type === 'movement_command') {
        await handleMovementCommand(data);
    } else if (event_type === 'system_status') {
        await handleSystemStatus(data);
    } else if (event_type === 'ble_status' || event_type === 'wifi_status') {
        await handleConnectionEvent(data);
    } else if (event_type === 'session_start') {
        await handleSessionStart(data);
    } else if (event_type === 'session_end') {
        await handleSessionEnd(data);
    } else if (event_type === 'session_progress') {
        await handleSessionProgress(data);
    } else {
        console.log(`ℹ️  Unhandled event type: ${event_type}`);
    }
}

// Handle movement command events
async function handleMovementCommand(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { command, response_time_ms, ble_connected, session_id } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, command,
                response_time_ms, servo_data, movement_successful, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, ?, ?, ?, NOW())
        `, [
            session_id || null,
            timestamp,
            'movement_command',
            command,
            response_time_ms,
            JSON.stringify({ ble_connected }),
            true // Assume successful unless we get error data
        ]);

        console.log(`✅ Stored movement command: ${command}`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('movement_command', data);
        }

    } catch (error) {
        console.error('❌ Error storing movement command:', error);
    }
}

// Handle system status updates
async function handleSystemStatus(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const {
            status, firmware_version, uptime_seconds,
            free_heap, wifi_connected, ble_connected,
            wifi_rssi, ip_address
        } = eventData;

        // Update system_status table (UPDATE only, don't insert duplicates)
        await dbPool.execute(`
            INSERT INTO system_status (
                device_id, status, last_seen, ip_address,
                firmware_version, uptime_seconds, created_at
            ) VALUES (?, ?, FROM_UNIXTIME(?), ?, ?, ?, NOW())
            ON DUPLICATE KEY UPDATE
                status = VALUES(status),
                last_seen = VALUES(last_seen),
                ip_address = VALUES(ip_address),
                uptime_seconds = VALUES(uptime_seconds),
                firmware_version = VALUES(firmware_version)
        `, [
            device_id,
            status,
            timestamp,
            ip_address || null,
            firmware_version,
            uptime_seconds
        ]);

        console.log(`✅ Updated system status: ${status}`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('system_status', data);
        }

    } catch (error) {
        console.error('❌ Error storing system status:', error);
    }
}

// Handle connection events (BLE/WiFi)
async function handleConnectionEvent(data) {
    try {
        const { device_id, timestamp, event_type, data: eventData } = data;
        const { status } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            null,
            timestamp,
            event_type === 'ble_status' ? 'ble_connect' : 'wifi_connect',
            JSON.stringify({ status, device_id })
        ]);

        console.log(`✅ Stored connection event: ${event_type} = ${status}`);

    } catch (error) {
        console.error('❌ Error storing connection event:', error);
    }
}

// Handle session start events
async function handleSessionStart(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { session_id, session_type, ble_connected, auto_started } = eventData;

        // Insert new session into sessions table
        await dbPool.execute(`
            INSERT INTO sessions (
                session_id, device_id, start_time, session_type,
                session_status, total_movements, successful_movements,
                total_cycles, created_at
            ) VALUES (?, ?, FROM_UNIXTIME(?), ?, 'active', 0, 0, 0, NOW())
        `, [
            session_id,
            device_id,
            timestamp,
            session_type
        ]);

        // Insert session start event
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id,
            timestamp,
            'session_start',
            JSON.stringify({ ble_connected, auto_started })
        ]);

        console.log(`✅ Session started: ${session_id} (${session_type})`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('session_start', data);
        }

    } catch (error) {
        console.error('❌ Error storing session start:', error);
    }
}

// Handle session end events
async function handleSessionEnd(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const {
            session_id, session_type, end_reason, total_duration,
            movements_completed, successful_movements, cycles_completed
        } = eventData;

        // Update session in sessions table
        await dbPool.execute(`
            UPDATE sessions SET
                end_time = FROM_UNIXTIME(?),
                duration_seconds = ?,
                session_status = ?,
                total_movements = ?,
                successful_movements = ?,
                total_cycles = ?,
                end_reason = ?
            WHERE session_id = ?
        `, [
            timestamp,
            Math.floor(total_duration / 1000), // Convert ms to seconds
            end_reason === 'user_requested' ? 'completed' : 'interrupted',
            movements_completed,
            successful_movements,
            cycles_completed,
            end_reason,
            session_id
        ]);

        // Insert session end event
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id,
            timestamp,
            'session_end',
            JSON.stringify({
                end_reason, total_duration, movements_completed,
                successful_movements, cycles_completed
            })
        ]);

        console.log(`✅ Session ended: ${session_id} (Duration: ${Math.floor(total_duration / 1000)}s, Movements: ${movements_completed})`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('session_end', data);
        }

    } catch (error) {
        console.error('❌ Error storing session end:', error);
    }
}

// Handle session progress events
async function handleSessionProgress(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const {
            session_id, completed_cycles, total_cycles,
            progress_percent, movements_completed
        } = eventData;

        // Update session progress (optional - for real-time tracking)
        await dbPool.execute(`
            UPDATE sessions SET
                total_movements = ?,
                total_cycles = ?
            WHERE session_id = ? AND session_status = 'active'
        `, [
            movements_completed,
            completed_cycles,
            session_id
        ]);

        console.log(`📊 Session progress: ${session_id} (${progress_percent.toFixed(1)}% - ${completed_cycles}/${total_cycles} cycles)`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('session_progress', data);
        }

    } catch (error) {
        console.error('❌ Error storing session progress:', error);
    }
}

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down MQTT bridge...');

    if (mqttClient) {
        mqttClient.end();
    }

    if (dbPool) {
        dbPool.end();
    }

    process.exit(0);
});

// Start the bridge
async function start() {
    console.log('🚀 Starting MQTT to Database Bridge...');

    await initDatabase();
    initMQTT();
    initWebSocket();

    console.log('✅ MQTT Bridge running with WebSocket support - Press Ctrl+C to stop');
}

start().catch(error => {
    console.error('❌ Failed to start MQTT bridge:', error);
    process.exit(1);
});
