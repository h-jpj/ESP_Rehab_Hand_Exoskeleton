# ESP32 Data Collection Enhancement Plan

## 🎯 Current State vs Required Data

### ✅ Currently Collecting:
- Basic system status (uptime, free heap, connectivity)
- Session start/end events
- Movement command reception
- Basic session tracking (total movements, session type)

### ❌ Missing for Grafana Dashboards:

#### **Row 1: Overview Stats**
- ✅ Active Sessions (have basic session tracking)
- ✅ Today's Sessions (have session timestamps)
- ❌ **Success Rate** - Need individual movement success/failure tracking
- ✅ System Status (have basic device status)

#### **Row 2: Session Analytics**
- ✅ Session Activity Over Time (have session timestamps)
- ✅ Session Types Distribution (have session type detection)

#### **Row 3: Performance Metrics**
- ❌ **Success Rate Trends** - Need detailed movement success tracking
- ❌ **Response Times** - Need per-movement timing data

#### **Row 4: System Health**
- ❌ **Memory Usage Trends** - Need historical memory tracking
- ❌ **Connection Status Timeline** - Need detailed connection quality metrics

#### **Row 5: Clinical Data**
- ❌ **Therapy Progress** - Need movement quality metrics
- ❌ **Movement Commands Distribution** - Need detailed command analytics

## 🚀 Enhancement Implementation Plan

### **Phase 1: Movement Analytics Enhancement**

#### **1.1 Individual Movement Tracking**
- Track each servo movement (start, duration, completion)
- Record success/failure for each movement
- Measure response times per movement
- Track movement quality metrics

#### **1.2 Enhanced MQTT Publishing**
- Publish detailed movement data per servo action
- Include timing, success status, and quality metrics
- Add movement completion events

### **Phase 2: Performance Monitoring**

#### **2.1 System Performance Metrics**
- Track loop performance and timing
- Monitor memory usage trends
- Record connection quality metrics
- Measure servo response times

#### **2.2 Connection Quality Tracking**
- WiFi signal strength trends
- BLE connection stability
- Connection drop/reconnect events
- Network latency measurements

### **Phase 3: Clinical Analytics**

#### **3.1 Movement Quality Assessment**
- Calculate movement smoothness
- Track range of motion consistency
- Detect movement anomalies
- Assess therapy effectiveness

#### **3.2 Progress Tracking**
- Session quality scores
- Improvement indicators
- Therapy goal progress
- Long-term trend analysis

## 📋 Detailed Implementation Steps

### **Step 1: Enhance ServoController**
- Add individual servo movement tracking
- Implement movement success/failure detection
- Add timing measurements for each movement
- Create movement quality assessment

### **Step 2: Expand MQTT Publishing**
- Add new MQTT topics for detailed analytics
- Publish per-movement data
- Include performance metrics
- Add clinical progress indicators

### **Step 3: Enhance SessionManager**
- Track detailed session analytics
- Calculate session quality scores
- Monitor therapy progress
- Record clinical metrics

### **Step 4: Improve SystemMonitor**
- Add memory usage trending
- Track connection quality metrics
- Monitor system performance
- Record health indicators

## 🎯 New MQTT Topics to Add

```
rehab_exo/ESP32_001/movement/individual    # Per-servo movement data
rehab_exo/ESP32_001/movement/quality       # Movement quality metrics
rehab_exo/ESP32_001/performance/timing     # System performance data
rehab_exo/ESP32_001/performance/memory     # Memory usage trends
rehab_exo/ESP32_001/clinical/progress      # Therapy progress data
rehab_exo/ESP32_001/clinical/quality       # Session quality scores
```

## 📊 New Data Fields to Collect

### **Movement Data:**
- Individual servo positions and timing
- Movement success/failure per servo
- Response time per movement
- Movement smoothness metrics

### **Performance Data:**
- Loop execution times
- Memory usage over time
- Connection quality metrics
- System health indicators

### **Clinical Data:**
- Session quality scores
- Movement consistency metrics
- Therapy progress indicators
- Goal achievement tracking
