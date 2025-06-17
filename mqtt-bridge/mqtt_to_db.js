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
    'rehab_exo/ESP32_001/session/progress',
    // Enhanced Analytics Topics
    'rehab_exo/ESP32_001/movement/individual',
    'rehab_exo/ESP32_001/movement/quality',
    'rehab_exo/ESP32_001/performance/timing',
    'rehab_exo/ESP32_001/performance/memory',
    'rehab_exo/ESP32_001/clinical/progress',
    'rehab_exo/ESP32_001/clinical/quality',
    // Biometric Topics
    'rehab_exo/ESP32_001/sensors/heart_rate',
    'rehab_exo/pulse_metrics'
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
    } else if (event_type === 'movement_individual') {
        await handleMovementIndividual(data);
    } else if (event_type === 'movement_quality') {
        await handleMovementQuality(data);
    } else if (event_type === 'performance_timing') {
        await handlePerformanceTiming(data);
    } else if (event_type === 'performance_memory') {
        await handlePerformanceMemory(data);
    } else if (event_type === 'clinical_progress') {
        await handleClinicalProgress(data);
    } else if (event_type === 'clinical_quality') {
        await handleClinicalQuality(data);
    } else if (topic.includes('sensors/heart_rate')) {
        await handleHeartRate(data);
    } else if (topic.includes('pulse_metrics')) {
        await handlePulseMetrics(data);
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

// Handle individual movement events
async function handleMovementIndividual(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const {
            servo_index, start_time, duration_ms, successful,
            start_angle, target_angle, actual_angle, smoothness,
            movement_type, session_id
        } = eventData;

        // Insert into events table with enhanced movement data
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, command,
                response_time_ms, servo_data, movement_successful, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, ?, ?, ?, NOW())
        `, [
            session_id || null,
            timestamp,
            'movement_individual',
            `servo_${servo_index}`,
            duration_ms,
            JSON.stringify({
                servo_index, start_time, start_angle, target_angle,
                actual_angle, smoothness, movement_type
            }),
            successful
        ]);

        console.log(`✅ Stored individual movement: Servo ${servo_index}, Duration ${duration_ms}ms`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('movement_individual', data);
        }

    } catch (error) {
        console.error('❌ Error storing individual movement:', error);
    }
}

// Handle movement quality events
async function handleMovementQuality(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { session_id, overall_quality, average_smoothness, success_rate } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id || null,
            timestamp,
            'movement_quality',
            JSON.stringify({ overall_quality, average_smoothness, success_rate })
        ]);

        console.log(`✅ Stored movement quality: Session ${session_id}, Quality ${overall_quality}`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('movement_quality', data);
        }

    } catch (error) {
        console.error('❌ Error storing movement quality:', error);
    }
}

// Handle performance timing events
async function handlePerformanceTiming(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { loop_time, average_loop_time, max_loop_time } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            null, // Performance events are not session-specific
            timestamp,
            'performance_timing',
            JSON.stringify({ loop_time, average_loop_time, max_loop_time })
        ]);

        console.log(`✅ Stored performance timing: Loop ${loop_time}ms, Avg ${average_loop_time}ms`);

    } catch (error) {
        console.error('❌ Error storing performance timing:', error);
    }
}

// Handle performance memory events
async function handlePerformanceMemory(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { free_heap, min_free_heap, memory_usage_percent } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            null, // Performance events are not session-specific
            timestamp,
            'performance_memory',
            JSON.stringify({ free_heap, min_free_heap, memory_usage_percent })
        ]);

        console.log(`✅ Stored performance memory: Free ${free_heap} bytes, Usage ${memory_usage_percent}%`);

    } catch (error) {
        console.error('❌ Error storing performance memory:', error);
    }
}

// Handle clinical progress events
async function handleClinicalProgress(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { session_id, progress_score, progress_indicators } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id || null,
            timestamp,
            'clinical_progress',
            JSON.stringify({ progress_score, progress_indicators })
        ]);

        console.log(`✅ Stored clinical progress: Session ${session_id}, Score ${progress_score}`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('clinical_progress', data);
        }

    } catch (error) {
        console.error('❌ Error storing clinical progress:', error);
    }
}

// Handle clinical quality events
async function handleClinicalQuality(data) {
    try {
        const { device_id, timestamp, data: eventData } = data;
        const { session_id, quality_score, quality_metrics } = eventData;

        // Insert into events table
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id || null,
            timestamp,
            'clinical_quality',
            JSON.stringify({ quality_score, quality_metrics })
        ]);

        console.log(`✅ Stored clinical quality: Session ${session_id}, Quality ${quality_score}`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('clinical_quality', data);
        }

    } catch (error) {
        console.error('❌ Error storing clinical quality:', error);
    }
}

// Handle heart rate sensor data
async function handleHeartRate(data) {
    try {
        const { device_id, timestamp, heart_rate, spo2, signal_quality, finger_detected, session_id } = data;

        // Insert into events table as biometric data
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id || null,
            timestamp / 1000, // Convert milliseconds to seconds
            'heart_rate',
            JSON.stringify({
                heart_rate,
                spo2,
                signal_quality,
                finger_detected,
                device_id
            })
        ]);

        console.log(`✅ Stored heart rate: ${heart_rate} BPM, SpO2: ${spo2}%, Quality: ${signal_quality}`);

        // Emit real-time update via WebSocket
        console.log(`🔍 WebSocket debug: socketClient=${!!socketClient}, connected=${socketClient?.connected}`);
        if (socketClient && socketClient.connected) {
            console.log(`📡 Emitting heart rate WebSocket event:`, data);
            socketClient.emit('heart_rate', data);
        } else {
            console.log(`❌ WebSocket not connected - cannot emit heart rate event`);
        }

    } catch (error) {
        console.error('❌ Error storing heart rate data:', error);
    }
}

// Handle pulse metrics data
async function handlePulseMetrics(data) {
    try {
        const { device_id, timestamp, session_id, avg_heart_rate, min_heart_rate, max_heart_rate, avg_spo2, data_quality } = data;

        // Insert into events table as pulse metrics
        await dbPool.execute(`
            INSERT INTO events (
                session_id, timestamp, event_type, servo_data, created_at
            ) VALUES (?, FROM_UNIXTIME(?), ?, ?, NOW())
        `, [
            session_id || null,
            timestamp / 1000, // Convert milliseconds to seconds
            'pulse_metrics',
            JSON.stringify({
                avg_heart_rate,
                min_heart_rate,
                max_heart_rate,
                avg_spo2,
                data_quality,
                device_id
            })
        ]);

        console.log(`✅ Stored pulse metrics: Session ${session_id}, Avg HR: ${avg_heart_rate} BPM`);

        // Emit real-time update via WebSocket
        if (socketClient && socketClient.connected) {
            socketClient.emit('pulse_metrics', data);
        }

    } catch (error) {
        console.error('❌ Error storing pulse metrics:', error);
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
