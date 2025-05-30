const express = require('express');
const mysql = require('mysql2/promise');
const cors = require('cors');
const path = require('path');
const http = require('http');
const socketIo = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  }
});

const PORT = process.env.PORT || 3000;

// Database configuration
const dbConfig = {
  host: process.env.DB_HOST || 'mariadb',
  user: process.env.DB_USER || 'jay',
  password: process.env.DB_PASS || 'aes',
  database: process.env.DB_NAME || 'rehab_exoskeleton',
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
};

// Create database connection pool
const pool = mysql.createPool(dbConfig);

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Test database connection
async function testConnection() {
  try {
    const connection = await pool.getConnection();
    console.log('✅ Database connected successfully');
    connection.release();
  } catch (error) {
    console.error('❌ Database connection failed:', error.message);
  }
}

// API Routes

// Get all sessions
app.get('/api/sessions', async (req, res) => {
  try {
    const [rows] = await pool.execute(`
      SELECT
        id,
        session_id,
        device_id,
        user_id,
        start_time,
        end_time,
        duration_seconds,
        session_type,
        total_cycles,
        created_at
      FROM sessions
      ORDER BY start_time DESC
      LIMIT 50
    `);
    res.json(rows);
  } catch (error) {
    console.error('Error fetching sessions:', error);
    res.status(500).json({ error: 'Failed to fetch sessions' });
  }
});

// Get session details
app.get('/api/sessions/:sessionId', async (req, res) => {
  try {
    const { sessionId } = req.params;

    // Get session info
    const [sessionRows] = await pool.execute(
      'SELECT * FROM sessions WHERE session_id = ?',
      [sessionId]
    );

    if (sessionRows.length === 0) {
      return res.status(404).json({ error: 'Session not found' });
    }

    // Get session events
    const [eventRows] = await pool.execute(`
      SELECT
        id,
        timestamp,
        event_type,
        command,
        response_time_ms,
        servo_data,
        error_message
      FROM events
      WHERE session_id = ?
      ORDER BY timestamp ASC
    `, [sessionId]);

    res.json({
      session: sessionRows[0],
      events: eventRows
    });
  } catch (error) {
    console.error('Error fetching session details:', error);
    res.status(500).json({ error: 'Failed to fetch session details' });
  }
});

// Get recent events
app.get('/api/events', async (req, res) => {
  try {
    const limit = req.query.limit || 20;
    const [rows] = await pool.execute(`
      SELECT
        e.id,
        e.session_id,
        e.timestamp,
        e.event_type,
        e.command,
        e.response_time_ms,
        s.session_type,
        s.device_id
      FROM events e
      LEFT JOIN sessions s ON e.session_id = s.session_id
      ORDER BY e.timestamp DESC
      LIMIT ?
    `, [parseInt(limit)]);
    res.json(rows);
  } catch (error) {
    console.error('Error fetching events:', error);
    res.status(500).json({ error: 'Failed to fetch events' });
  }
});

// Get system status
app.get('/api/status', async (req, res) => {
  try {
    const [statusRows] = await pool.execute(
      'SELECT * FROM system_status ORDER BY last_seen DESC'
    );

    // Get session statistics
    const [statsRows] = await pool.execute(`
      SELECT
        COUNT(*) as total_sessions,
        SUM(duration_seconds) as total_duration,
        AVG(duration_seconds) as avg_duration,
        SUM(total_cycles) as total_cycles
      FROM sessions
    `);

    // Get today's sessions
    const [todayRows] = await pool.execute(`
      SELECT COUNT(*) as today_sessions
      FROM sessions
      WHERE DATE(start_time) = CURDATE()
    `);

    res.json({
      devices: statusRows,
      statistics: {
        ...statsRows[0],
        today_sessions: todayRows[0].today_sessions
      }
    });
  } catch (error) {
    console.error('Error fetching status:', error);
    res.status(500).json({ error: 'Failed to fetch status' });
  }
});

// WebSocket connection for real-time updates
io.on('connection', (socket) => {
  console.log('Client connected for real-time updates');

  socket.on('disconnect', () => {
    console.log('Client disconnected');
  });
});

// Function to broadcast real-time updates (will be used when ESP32 sends data)
function broadcastUpdate(type, data) {
  io.emit('update', { type, data, timestamp: new Date() });
}

// Serve the main HTML file
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Start server
server.listen(PORT, () => {
  console.log(`🚀 Rehab Exoskeleton Web App running on port ${PORT}`);
  console.log(`📊 Dashboard: http://localhost:${PORT}`);
  console.log(`🔗 API: http://localhost:${PORT}/api/sessions`);
  testConnection();
});

module.exports = { app, broadcastUpdate };
