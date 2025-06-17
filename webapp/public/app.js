// ESP32 Rehabilitation Dashboard - Frontend JavaScript

class RehabDashboard {
    constructor() {
        this.socket = io();
        this.init();
        this.setupEventListeners();
        this.startDataRefresh();
    }

    init() {
        console.log('🚀 Rehab Dashboard initialized');
        this.loadInitialData();
    }

    setupEventListeners() {
        // Socket.io events
        this.socket.on('connect', () => {
            this.updateConnectionStatus(true);
        });

        this.socket.on('disconnect', () => {
            this.updateConnectionStatus(false);
        });

        this.socket.on('system_status', (data) => {
            this.handleRealTimeUpdate({...data, type: 'system_status'});
        });

        this.socket.on('movement_command', (data) => {
            this.handleRealTimeUpdate({...data, type: 'movement_command'});
        });

        this.socket.on('heart_rate', (data) => {
            this.updateBiometricData(data);
        });

        // Modal events
        const modal = document.getElementById('sessionModal');
        const closeModal = document.getElementById('closeModal');

        closeModal.onclick = () => {
            modal.style.display = 'none';
        };

        window.onclick = (event) => {
            if (event.target === modal) {
                modal.style.display = 'none';
            }
        };
    }

    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connectionStatus');
        const dot = statusElement.querySelector('.status-dot');
        const text = statusElement.querySelector('span:last-child');

        if (connected) {
            dot.className = 'status-dot online';
            text.textContent = 'Connected';
        } else {
            dot.className = 'status-dot offline';
            text.textContent = 'Disconnected';
        }
    }

    async loadInitialData() {
        try {
            await Promise.all([
                this.loadSystemStatus(),
                this.loadSessions(),
                this.loadRecentEvents()
            ]);
            this.initializeBiometricDisplay();
        } catch (error) {
            console.error('Error loading initial data:', error);
        }
    }

    initializeBiometricDisplay() {
        // Initialize biometric display with default values
        this.updateBiometricData({
            heart_rate: 0,
            spo2: 0,
            signal_quality: 'no_signal',
            finger_detected: false,
            session_id: ''
        });

        // Update status indicators
        document.getElementById('sensorStatus').textContent = 'Disconnected';
        document.getElementById('sensorStatus').className = 'status-value disconnected';
        document.getElementById('lastReading').textContent = 'Never';
        document.getElementById('sessionActive').textContent = 'No';
        document.getElementById('sessionActive').className = 'status-value';
    }

    updateBiometricData(data) {
        console.log('Biometric update:', data);

        // Update heart rate
        const heartRateElement = document.getElementById('heartRate');
        const hrQualityElement = document.getElementById('hrQuality');
        const hrTrendElement = document.getElementById('hrTrend');

        if (data.finger_detected && data.heart_rate > 0) {
            heartRateElement.textContent = Math.round(data.heart_rate);
            heartRateElement.parentElement.parentElement.classList.add('pulse-animation');
        } else {
            heartRateElement.textContent = '--';
            heartRateElement.parentElement.parentElement.classList.remove('pulse-animation');
        }

        // Update SpO2
        const spO2Element = document.getElementById('spO2');
        const fingerStatusElement = document.getElementById('fingerStatus');
        const spo2TrendElement = document.getElementById('spo2Trend');

        if (data.finger_detected && data.spo2 > 0) {
            spO2Element.textContent = Math.round(data.spo2);
        } else {
            spO2Element.textContent = '--';
        }

        // Update signal quality
        hrQualityElement.textContent = data.signal_quality.replace('_', ' ');
        hrQualityElement.className = `signal-quality ${data.signal_quality}`;

        // Update finger status
        fingerStatusElement.textContent = data.finger_detected ? 'Detected' : 'Not Detected';
        fingerStatusElement.className = `finger-status ${data.finger_detected ? 'detected' : 'not-detected'}`;

        // Update sensor status
        const sensorStatusElement = document.getElementById('sensorStatus');
        if (data.finger_detected || data.heart_rate > 0 || data.spo2 > 0) {
            sensorStatusElement.textContent = 'Connected';
            sensorStatusElement.className = 'status-value connected';
        } else {
            sensorStatusElement.textContent = 'No Signal';
            sensorStatusElement.className = 'status-value disconnected';
        }

        // Update last reading time
        document.getElementById('lastReading').textContent = new Date().toLocaleTimeString();

        // Update session status
        const sessionActiveElement = document.getElementById('sessionActive');
        if (data.session_id && data.session_id !== '') {
            sessionActiveElement.textContent = 'Yes';
            sessionActiveElement.className = 'status-value active';
        } else {
            sessionActiveElement.textContent = 'No';
            sessionActiveElement.className = 'status-value';
        }

        // Store previous values for trend calculation
        if (!this.previousBiometrics) {
            this.previousBiometrics = {};
        }

        // Calculate and display trends
        this.updateBiometricTrends(data);

        this.previousBiometrics = data;
    }

    updateBiometricTrends(currentData) {
        if (!this.previousBiometrics) return;

        // Heart rate trend
        const hrTrendElement = document.getElementById('hrTrend').querySelector('.trend-indicator');
        if (currentData.heart_rate > 0 && this.previousBiometrics.heart_rate > 0) {
            const hrDiff = currentData.heart_rate - this.previousBiometrics.heart_rate;
            if (Math.abs(hrDiff) < 2) {
                hrTrendElement.textContent = '→ Stable';
                hrTrendElement.className = 'trend-indicator stable';
            } else if (hrDiff > 0) {
                hrTrendElement.textContent = '↗ Rising';
                hrTrendElement.className = 'trend-indicator up';
            } else {
                hrTrendElement.textContent = '↘ Falling';
                hrTrendElement.className = 'trend-indicator down';
            }
        } else {
            hrTrendElement.textContent = '--';
            hrTrendElement.className = 'trend-indicator';
        }

        // SpO2 trend
        const spo2TrendElement = document.getElementById('spo2Trend').querySelector('.trend-indicator');
        if (currentData.spo2 > 0 && this.previousBiometrics.spo2 > 0) {
            const spo2Diff = currentData.spo2 - this.previousBiometrics.spo2;
            if (Math.abs(spo2Diff) < 1) {
                spo2TrendElement.textContent = '→ Stable';
                spo2TrendElement.className = 'trend-indicator stable';
            } else if (spo2Diff > 0) {
                spo2TrendElement.textContent = '↗ Rising';
                spo2TrendElement.className = 'trend-indicator up';
            } else {
                spo2TrendElement.textContent = '↘ Falling';
                spo2TrendElement.className = 'trend-indicator down';
            }
        } else {
            spo2TrendElement.textContent = '--';
            spo2TrendElement.className = 'trend-indicator';
        }
    }

    async loadSystemStatus() {
        try {
            const response = await fetch('/api/status');
            const data = await response.json();

            // Update statistics
            document.getElementById('totalSessions').textContent = data.statistics.total_sessions || 0;
            document.getElementById('todaySessions').textContent = data.statistics.today_sessions || 0;
            document.getElementById('totalDuration').textContent = this.formatDuration(data.statistics.total_duration || 0);
            document.getElementById('totalCycles').textContent = data.statistics.total_cycles || 0;

            // Update device status
            this.updateDeviceStatus(data.devices);

        } catch (error) {
            console.error('Error loading system status:', error);
            this.showError('Failed to load system status');
        }
    }

    updateDeviceStatus(devices) {
        const container = document.getElementById('deviceStatus');

        if (!devices || devices.length === 0) {
            container.innerHTML = '<div class="info-message">No devices found</div>';
            return;
        }

        container.innerHTML = devices.map(device => `
            <div class="device-card ${device.status}">
                <div class="device-header">
                    <h4>${device.device_id}</h4>
                    <span class="status-badge ${device.status}">${device.status}</span>
                </div>
                <div class="device-details">
                    <p><strong>Last Seen:</strong> ${this.formatDateTime(device.last_seen)}</p>
                    <p><strong>Firmware:</strong> ${device.firmware_version || 'Unknown'}</p>
                    ${device.ip_address ? `<p><strong>IP:</strong> ${device.ip_address}</p>` : ''}
                    ${device.uptime_seconds ? `<p><strong>Uptime:</strong> ${this.formatDuration(device.uptime_seconds)}</p>` : ''}
                </div>
            </div>
        `).join('');
    }

    async loadSessions() {
        try {
            const response = await fetch('/api/sessions');
            const sessions = await response.json();

            const tbody = document.getElementById('sessionsTableBody');

            if (sessions.length === 0) {
                tbody.innerHTML = '<tr><td colspan="7" class="info-message">No sessions found</td></tr>';
                return;
            }

            tbody.innerHTML = sessions.map(session => `
                <tr>
                    <td class="session-id">${session.session_id}</td>
                    <td>${session.device_id}</td>
                    <td>
                        <span class="session-type ${session.session_type}">${session.session_type}</span>
                        <br>
                        <small class="session-status ${session.session_status}">${session.session_status}</small>
                    </td>
                    <td>${this.formatDateTime(session.start_time)}</td>
                    <td>${this.formatDuration(session.duration_seconds)}</td>
                    <td>
                        ${session.total_movements || 0}
                        ${session.successful_movements !== undefined ?
                            `<br><small>${session.successful_movements}/${session.total_movements} successful</small>` : ''}
                    </td>
                    <td>${session.total_cycles || 0}</td>
                    <td>
                        <button class="btn-small" onclick="dashboard.viewSessionDetails('${session.session_id}')">
                            View Details
                        </button>
                    </td>
                </tr>
            `).join('');

        } catch (error) {
            console.error('Error loading sessions:', error);
            this.showError('Failed to load sessions');
        }
    }

    async loadRecentEvents() {
        try {
            const response = await fetch('/api/events?limit=10');
            const events = await response.json();

            const container = document.getElementById('eventsList');

            if (events.length === 0) {
                container.innerHTML = '<div class="info-message">No recent events</div>';
                return;
            }

            container.innerHTML = events.map(event => `
                <div class="event-item ${event.event_type}">
                    <div class="event-header">
                        <span class="event-type">${this.formatEventType(event.event_type)}</span>
                        <span class="event-time">${this.formatDateTime(event.timestamp)}</span>
                    </div>
                    <div class="event-details">
                        ${event.session_id ? `<span class="session-ref">Session: ${event.session_id}</span>` : ''}
                        ${event.command ? `<span class="command">Command: ${event.command}</span>` : ''}
                        ${event.response_time_ms ? `<span class="response-time">${event.response_time_ms}ms</span>` : ''}
                    </div>
                </div>
            `).join('');

        } catch (error) {
            console.error('Error loading recent events:', error);
            this.showError('Failed to load recent events');
        }
    }

    async viewSessionDetails(sessionId) {
        try {
            const response = await fetch(`/api/sessions/${sessionId}`);
            const data = await response.json();

            const modal = document.getElementById('sessionModal');
            const detailsContainer = document.getElementById('sessionDetails');

            detailsContainer.innerHTML = `
                <div class="session-info">
                    <h4>Session Information</h4>
                    <div class="info-grid">
                        <div><strong>Session ID:</strong> ${data.session.session_id}</div>
                        <div><strong>Device:</strong> ${data.session.device_id}</div>
                        <div><strong>Type:</strong> <span class="session-type ${data.session.session_type}">${data.session.session_type}</span></div>
                        <div><strong>User:</strong> ${data.session.user_id || 'Unknown'}</div>
                        <div><strong>Start Time:</strong> ${this.formatDateTime(data.session.start_time)}</div>
                        <div><strong>End Time:</strong> ${data.session.end_time ? this.formatDateTime(data.session.end_time) : 'Ongoing'}</div>
                        <div><strong>Duration:</strong> ${this.formatDuration(data.session.duration_seconds)}</div>
                        <div><strong>Status:</strong> <span class="session-status ${data.session.session_status}">${data.session.session_status}</span></div>
                        <div><strong>Total Movements:</strong> ${data.session.total_movements || 0}</div>
                        <div><strong>Successful Movements:</strong> ${data.session.successful_movements || 0}</div>
                        <div><strong>Success Rate:</strong> ${data.session.total_movements > 0 ?
                            ((data.session.successful_movements / data.session.total_movements) * 100).toFixed(1) + '%' : 'N/A'}</div>
                        <div><strong>Total Cycles:</strong> ${data.session.total_cycles || 0}</div>
                        ${data.session.end_reason ? `<div><strong>End Reason:</strong> ${data.session.end_reason}</div>` : ''}
                    </div>
                </div>

                <div class="session-events">
                    <h4>Session Events (${data.events.length})</h4>
                    <div class="events-timeline">
                        ${data.events.map(event => `
                            <div class="timeline-event ${event.event_type}">
                                <div class="event-time">${this.formatTime(event.timestamp)}</div>
                                <div class="event-content">
                                    <strong>${this.formatEventType(event.event_type)}</strong>
                                    ${event.command ? `<br>Command: ${event.command}` : ''}
                                    ${event.response_time_ms ? `<br>Response: ${event.response_time_ms}ms` : ''}
                                    ${event.error_message ? `<br><span class="error">Error: ${event.error_message}</span>` : ''}
                                </div>
                            </div>
                        `).join('')}
                    </div>
                </div>
            `;

            modal.style.display = 'block';

        } catch (error) {
            console.error('Error loading session details:', error);
            this.showError('Failed to load session details');
        }
    }

    handleRealTimeUpdate(data) {
        console.log('Real-time update:', data);

        const container = document.getElementById('realtimeEvents');
        const eventElement = document.createElement('div');
        eventElement.className = `realtime-event ${data.type}`;
        eventElement.innerHTML = `
            <div class="event-header">
                <span class="event-type">${this.formatEventType(data.type)}</span>
                <span class="event-time">${this.formatDateTime(data.timestamp)}</span>
            </div>
            <div class="event-data">${JSON.stringify(data.data, null, 2)}</div>
        `;

        // Add to top of container
        container.insertBefore(eventElement, container.firstChild);

        // Keep only last 10 events
        const events = container.querySelectorAll('.realtime-event');
        if (events.length > 10) {
            events[events.length - 1].remove();
        }

        // Update info message if this is the first event
        const infoMessage = container.querySelector('.info-message');
        if (infoMessage) {
            infoMessage.remove();
        }

        // Refresh data to show updates
        this.loadSystemStatus();
        this.loadSessions();
        this.loadRecentEvents();
    }

    startDataRefresh() {
        // Refresh data every 30 seconds
        setInterval(() => {
            this.loadInitialData();
        }, 30000);
    }

    // Utility functions
    formatDateTime(dateString) {
        if (!dateString) return 'N/A';
        return new Date(dateString).toLocaleString();
    }

    formatTime(dateString) {
        if (!dateString) return 'N/A';
        return new Date(dateString).toLocaleTimeString();
    }

    formatDuration(seconds) {
        if (!seconds || seconds === 0) return '0s';

        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;

        if (hours > 0) {
            return `${hours}h ${minutes}m ${secs}s`;
        } else if (minutes > 0) {
            return `${minutes}m ${secs}s`;
        } else {
            return `${secs}s`;
        }
    }

    formatEventType(eventType) {
        const types = {
            'session_start': '🟢 Session Started',
            'session_end': '🔴 Session Ended',
            'movement_command': '🎮 Movement Command',
            'movement_complete': '✅ Movement Complete',
            'ble_connect': '📱 BLE Connected',
            'ble_disconnect': '📱 BLE Disconnected',
            'system_error': '❌ System Error'
        };
        return types[eventType] || eventType;
    }

    showError(message) {
        console.error(message);
        // You could add a toast notification here
    }
}

// Initialize dashboard when page loads
let dashboard;
document.addEventListener('DOMContentLoaded', () => {
    dashboard = new RehabDashboard();
});
