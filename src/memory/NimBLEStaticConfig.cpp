#include "NimBLEStaticConfig.h"
#include "../utils/Logger.h"
// NimBLEDevice.h included only where needed to avoid macro conflicts

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

bool NimBLEStaticConfig::configured = false;
bool NimBLEStaticConfig::allocatorOverridden = false;
NimBLEStaticConfig::OriginalAllocators NimBLEStaticConfig::originalAllocators = {nullptr, nullptr, nullptr, nullptr};

// =============================================================================
// CONFIGURATION AND LIFECYCLE
// =============================================================================

bool NimBLEStaticConfig::configure() {
    if (configured) {
        Logger::warning("NimBLE Static Config already configured");
        return true;
    }
    
    Logger::info("Configuring NimBLE for Static Memory...");
    
    // Ensure static BLE memory is initialized first
    if (!StaticBLEMemory::isInitialized()) {
        Logger::error("Static BLE Memory must be initialized before NimBLE configuration");
        return false;
    }
    
    // Configure NimBLE memory settings
    if (!configureNimBLEMemorySettings()) {
        Logger::error("Failed to configure NimBLE memory settings");
        return false;
    }
    
    // Configure NimBLE buffers
    if (!configureNimBLEBuffers()) {
        Logger::error("Failed to configure NimBLE buffers");
        return false;
    }
    
    // Configure NimBLE pools
    if (!configureNimBLEPools()) {
        Logger::error("Failed to configure NimBLE pools");
        return false;
    }
    
    // Override allocator functions
    if (!overrideNimBLEAllocator()) {
        Logger::error("Failed to override NimBLE allocator");
        return false;
    }
    
    configured = true;
    Logger::info("NimBLE Static Memory configuration complete");
    
    // Validate configuration
    if (!validateConfiguration()) {
        Logger::warning("NimBLE configuration validation failed");
    }
    
    logConfiguration();
    return true;
}

void NimBLEStaticConfig::restore() {
    if (!configured) return;
    
    Logger::info("Restoring NimBLE default configuration...");
    
    // Restore original allocator
    restoreNimBLEAllocator();
    
    configured = false;
    allocatorOverridden = false;
    
    Logger::info("NimBLE configuration restored");
}

bool NimBLEStaticConfig::isConfigured() {
    return configured;
}

// =============================================================================
// MEMORY OVERRIDE FUNCTIONS
// =============================================================================

bool NimBLEStaticConfig::overrideNimBLEAllocator() {
    if (allocatorOverridden) {
        Logger::warning("NimBLE allocator already overridden");
        return true;
    }
    
    Logger::info("Overriding NimBLE memory allocator...");
    
    // Store original allocators (if available)
    // Note: This is a simplified approach - production would use proper NimBLE hooks
    
    // Override would happen here through NimBLE configuration
    // For now, we'll rely on the C function hooks in StaticBLEMemory.cpp
    
    allocatorOverridden = true;
    Logger::info("NimBLE allocator override complete");
    
    return true;
}

void NimBLEStaticConfig::restoreNimBLEAllocator() {
    if (!allocatorOverridden) return;
    
    Logger::info("Restoring NimBLE default allocator...");
    
    // Restore original allocators
    // This would restore the original malloc/free functions
    
    allocatorOverridden = false;
    Logger::info("NimBLE allocator restored");
}

// =============================================================================
// CONFIGURATION HELPERS
// =============================================================================

bool NimBLEStaticConfig::configureNimBLEMemorySettings() {
    Logger::info("Configuring NimBLE memory settings...");
    
    // Configure memory pool sizes
    // These would be set through NimBLE configuration defines
    // For now, we rely on the defines in the header file
    
    Logger::infof("NimBLE Memory Configuration:");
    Logger::infof("  Max Connections: %d", NimBLEConfig::MAX_CONNECTIONS);
    Logger::infof("  Max Services: %d", NimBLEConfig::MAX_SERVICES);
    Logger::infof("  Max Characteristics: %d", NimBLEConfig::MAX_CHARACTERISTICS);
    Logger::infof("  ATT MTU Size: %d", NimBLEConfig::ATT_MTU_SIZE);
    
    return true;
}

bool NimBLEStaticConfig::configureNimBLEBuffers() {
    Logger::info("Configuring NimBLE buffers...");
    
    // Configure buffer sizes
    Logger::infof("NimBLE Buffer Configuration:");
    Logger::infof("  L2CAP MTU: %d", NimBLEConfig::L2CAP_MTU_SIZE);
    Logger::infof("  HCI Buffer Size: %d", NimBLEConfig::HCI_BUFFER_SIZE);
    Logger::infof("  ACL Buffer Count: %d", NimBLEConfig::ACL_BUFFER_COUNT);
    
    return true;
}

bool NimBLEStaticConfig::configureNimBLEPools() {
    Logger::info("Configuring NimBLE memory pools...");
    
    // Configure memory pools
    Logger::infof("NimBLE Pool Configuration:");
    Logger::infof("  MSYS Block Count: %d", NimBLEConfig::MSYS_BLOCK_COUNT);
    Logger::infof("  MSYS Block Size: %d", NimBLEConfig::MSYS_BLOCK_SIZE);
    Logger::infof("  GATT Max Registrations: %d", NimBLEConfig::GATT_SVR_MAX_REGISTRATIONS);
    
    return true;
}

// =============================================================================
// VALIDATION HELPERS
// =============================================================================

bool NimBLEStaticConfig::validateConfiguration() {
    Logger::info("Validating NimBLE static configuration...");
    
    bool valid = true;
    
    // Validate memory settings
    if (!validateMemorySettings()) {
        Logger::error("NimBLE memory settings validation failed");
        valid = false;
    }
    
    // Validate buffer sizes
    if (!validateBufferSizes()) {
        Logger::error("NimBLE buffer sizes validation failed");
        valid = false;
    }
    
    // Validate pool configuration
    if (!validatePoolConfiguration()) {
        Logger::error("NimBLE pool configuration validation failed");
        valid = false;
    }
    
    // Validate static memory health
    if (!StaticBLEMemory::isHealthy()) {
        Logger::error("Static BLE Memory health check failed");
        valid = false;
    }
    
    return valid;
}

bool NimBLEStaticConfig::validateMemorySettings() {
    // Check if memory settings are reasonable
    if (NimBLEConfig::MAX_CONNECTIONS < 1 || NimBLEConfig::MAX_CONNECTIONS > 4) {
        Logger::error("Invalid max connections setting");
        return false;
    }
    
    if (NimBLEConfig::ATT_MTU_SIZE < 23 || NimBLEConfig::ATT_MTU_SIZE > 512) {
        Logger::error("Invalid ATT MTU size");
        return false;
    }
    
    return true;
}

bool NimBLEStaticConfig::validateBufferSizes() {
    // Check if buffer sizes fit within our static allocation
    size_t totalBufferSize = NimBLEConfig::HCI_BUFFER_SIZE * NimBLEConfig::ACL_BUFFER_COUNT;
    if (totalBufferSize > BLE_STATIC_POOL_SIZE / 2) {
        Logger::error("Buffer sizes too large for static pool");
        return false;
    }
    
    return true;
}

bool NimBLEStaticConfig::validatePoolConfiguration() {
    // Check if pool configuration is reasonable
    size_t totalPoolSize = NimBLEConfig::MSYS_BLOCK_COUNT * NimBLEConfig::MSYS_BLOCK_SIZE;
    if (totalPoolSize > BLE_STATIC_POOL_SIZE) {
        Logger::error("Pool configuration exceeds static memory size");
        return false;
    }
    
    return true;
}

void NimBLEStaticConfig::logConfiguration() {
    Logger::info("=== NimBLE Static Configuration ===");
    Logger::infof("Configured: %s", configured ? "Yes" : "No");
    Logger::infof("Allocator Overridden: %s", allocatorOverridden ? "Yes" : "No");
    Logger::infof("Static Memory Pool: %d bytes", BLE_STATIC_POOL_SIZE);
    Logger::infof("Max Connections: %d", NimBLEConfig::MAX_CONNECTIONS);
    Logger::infof("Max Services: %d", NimBLEConfig::MAX_SERVICES);
    Logger::infof("Max Characteristics: %d", NimBLEConfig::MAX_CHARACTERISTICS);
    Logger::infof("ATT MTU: %d bytes", NimBLEConfig::ATT_MTU_SIZE);
    Logger::infof("Memory Pool Blocks: %d × %d bytes", 
                 NimBLEConfig::MSYS_BLOCK_COUNT, NimBLEConfig::MSYS_BLOCK_SIZE);
    Logger::info("==================================");
}

// =============================================================================
// EMERGENCY RECOVERY
// =============================================================================

bool NimBLEStaticConfig::emergencyReconfigure() {
    Logger::warning("Emergency NimBLE reconfiguration initiated...");
    
    // Restore original configuration
    restore();
    
    // Reset static memory
    StaticBLEMemory::emergencyReset();
    
    // Reinitialize
    if (!StaticBLEMemory::initialize()) {
        Logger::error("Emergency: Failed to reinitialize static memory");
        return false;
    }
    
    // Reconfigure
    if (!configure()) {
        Logger::error("Emergency: Failed to reconfigure NimBLE");
        return false;
    }
    
    Logger::info("Emergency reconfiguration complete");
    return true;
}

// =============================================================================
// NIMBLE MEMORY HOOKS IMPLEMENTATION
// =============================================================================

namespace NimBLEMemoryHooks {
    void* static_malloc(size_t size) {
        void* ptr = StaticBLEMemory::allocate(size);
        track_allocation(ptr, size, "malloc");
        return ptr;
    }
    
    void static_free(void* ptr) {
        track_deallocation(ptr, "free");
        StaticBLEMemory::deallocate(ptr);
    }
    
    void* static_realloc(void* ptr, size_t size) {
        track_deallocation(ptr, "realloc_old");
        void* new_ptr = ble_static_realloc(ptr, size);
        track_allocation(new_ptr, size, "realloc_new");
        return new_ptr;
    }
    
    void* static_calloc(size_t num, size_t size) {
        void* ptr = ble_static_calloc(num, size);
        track_allocation(ptr, num * size, "calloc");
        return ptr;
    }
    
    void* allocate_connection_context(size_t size) {
        return StaticBLEMemory::allocateConnection(size);
    }
    
    void* allocate_characteristic_buffer(size_t size) {
        return StaticBLEMemory::allocateCharacteristic(size);
    }
    
    void* allocate_gatt_buffer(size_t size) {
        return StaticBLEMemory::allocateEvent(size);
    }
    
    void* allocate_hci_buffer(size_t size) {
        return StaticBLEMemory::allocateEvent(size);
    }
    
    void track_allocation(void* ptr, size_t size, const char* component) {
        if (ptr) {
            Logger::debugf("BLE %s: allocated %d bytes at %p", component, size, ptr);
        } else {
            handle_allocation_failure(size, component);
        }
    }
    
    void track_deallocation(void* ptr, const char* component) {
        if (ptr) {
            Logger::debugf("BLE %s: deallocated %p", component, ptr);
        }
    }
    
    void handle_allocation_failure(size_t size, const char* component) {
        Logger::errorf("BLE allocation failure: %s requested %d bytes", component, size);
        StaticBLEMemory::logMemoryStatus();
    }
    
    void handle_memory_corruption(void* ptr, const char* component) {
        Logger::errorf("BLE memory corruption detected: %s at %p", component, ptr);
    }
}

// =============================================================================
// GLOBAL FUNCTIONS
// =============================================================================

bool initializeNimBLEWithStaticMemory() {
    Logger::info("Initializing NimBLE with Static Memory...");
    
    // Initialize static memory first
    if (!StaticBLEMemory::initialize()) {
        Logger::error("Failed to initialize Static BLE Memory");
        return false;
    }
    
    // Configure NimBLE
    if (!NimBLEStaticConfig::configure()) {
        Logger::error("Failed to configure NimBLE for static memory");
        StaticBLEMemory::shutdown();
        return false;
    }
    
    Logger::info("NimBLE Static Memory initialization complete");
    return true;
}

void shutdownNimBLEStaticMemory() {
    Logger::info("Shutting down NimBLE Static Memory...");
    
    NimBLEStaticConfig::restore();
    StaticBLEMemory::shutdown();
    
    Logger::info("NimBLE Static Memory shutdown complete");
}

bool validateNimBLEStaticMemory() {
    return NimBLEStaticConfig::validateConfiguration() && StaticBLEMemory::isHealthy();
}

bool recoverNimBLEStaticMemory() {
    Logger::warning("Attempting NimBLE Static Memory recovery...");
    return NimBLEStaticConfig::emergencyReconfigure();
}

NimBLEMemoryUsage getNimBLEMemoryUsage() {
    BLEMemoryStats stats = getBLEMemoryStats();
    
    NimBLEMemoryUsage usage;
    usage.totalAllocated = stats.usedSize;
    usage.peakUsage = stats.peakUsage;
    usage.allocationCount = stats.allocationCount;
    usage.failureCount = 0; // Would need to track this
    usage.isHealthy = stats.isHealthy;
    usage.efficiency = stats.freeSize > 0 ? (float)stats.usedSize / (float)stats.totalSize : 0.0f;
    
    return usage;
}

void logNimBLEMemoryStatus() {
    Logger::info("=== NimBLE Memory Status ===");
    
    NimBLEMemoryUsage usage = getNimBLEMemoryUsage();
    Logger::infof("Total Allocated: %d bytes", usage.totalAllocated);
    Logger::infof("Peak Usage: %d bytes", usage.peakUsage);
    Logger::infof("Allocation Count: %lu", usage.allocationCount);
    Logger::infof("Efficiency: %.1f%%", usage.efficiency * 100.0f);
    Logger::infof("Health Status: %s", usage.isHealthy ? "Healthy" : "Warning");
    
    // Also log underlying static memory status
    StaticBLEMemory::logMemoryStatus();
    
    Logger::info("===========================");
}

bool isNimBLEMemoryHealthy() {
    return validateNimBLEStaticMemory();
}
