#ifndef NIMBLE_STATIC_CONFIG_H
#define NIMBLE_STATIC_CONFIG_H

#include "StaticBLEMemory.h"
// Note: NimBLEDevice.h included only where needed to avoid macro conflicts

// =============================================================================
// NIMBLE STATIC MEMORY CONFIGURATION
// =============================================================================

/**
 * This module configures NimBLE to use our static memory allocator
 * instead of the default heap allocator, eliminating fragmentation issues.
 */

class NimBLEStaticConfig {
public:
    // Configuration and lifecycle
    static bool configure();
    static void restore();
    static bool isConfigured();
    
    // Memory override functions
    static bool overrideNimBLEAllocator();
    static void restoreNimBLEAllocator();
    
    // Configuration validation
    static bool validateConfiguration();
    static void logConfiguration();
    
    // Emergency recovery
    static bool emergencyReconfigure();
    
private:
    static bool configured;
    static bool allocatorOverridden;
    
    // Store original allocator functions for restoration
    struct OriginalAllocators {
        void* (*malloc_func)(size_t);
        void (*free_func)(void*);
        void* (*realloc_func)(void*, size_t);
        void* (*calloc_func)(size_t, size_t);
    };
    
    static OriginalAllocators originalAllocators;
    
    // Configuration helpers
    static bool configureNimBLEMemorySettings();
    static bool configureNimBLEBuffers();
    static bool configureNimBLEPools();
    
    // Validation helpers
    static bool validateMemorySettings();
    static bool validateBufferSizes();
    static bool validatePoolConfiguration();
};

// =============================================================================
// NIMBLE MEMORY CONFIGURATION CONSTANTS
// =============================================================================

// NimBLE memory pool configurations optimized for static allocation
namespace NimBLEConfig {
    // Connection parameters
    static const int MAX_CONNECTIONS = 1;          // Single connection for medical device
    static const int MAX_SERVICES = 2;             // Device info + custom service
    static const int MAX_CHARACTERISTICS = 4;      // Command, status, data, config
    static const int MAX_DESCRIPTORS = 8;          // CCCD and user descriptors
    
    // Buffer sizes (optimized for our static pools)
    static const int ATT_MTU_SIZE = 247;            // Maximum ATT MTU
    static const int L2CAP_MTU_SIZE = 256;          // L2CAP MTU
    static const int HCI_BUFFER_SIZE = 260;         // HCI buffer size
    static const int ACL_BUFFER_COUNT = 4;          // Number of ACL buffers
    
    // Memory pool sizes (must fit within our static allocation)
    static const int MSYS_BLOCK_COUNT = 32;        // Memory system blocks
    static const int MSYS_BLOCK_SIZE = 256;        // Size of each block
    static const int GATT_SVR_MAX_REGISTRATIONS = 8; // GATT server registrations
    
    // Security and pairing
    static const bool ENABLE_SECURITY = false;     // Disable for simplicity
    static const bool ENABLE_BONDING = false;      // Disable bonding
    
    // Power management
    static const int TX_POWER_LEVEL = 9;           // Maximum TX power
    static const bool ENABLE_SLEEP = false;        // Disable sleep for reliability
}

// =============================================================================
// MEMORY ALLOCATION HOOKS
// =============================================================================

/**
 * Custom memory allocation functions that redirect NimBLE
 * memory requests to our static memory pools.
 */
namespace NimBLEMemoryHooks {
    // Primary allocation functions
    void* static_malloc(size_t size);
    void static_free(void* ptr);
    void* static_realloc(void* ptr, size_t size);
    void* static_calloc(size_t num, size_t size);
    
    // Specialized allocation functions for different NimBLE components
    void* allocate_connection_context(size_t size);
    void* allocate_characteristic_buffer(size_t size);
    void* allocate_gatt_buffer(size_t size);
    void* allocate_hci_buffer(size_t size);
    
    // Memory tracking and debugging
    void track_allocation(void* ptr, size_t size, const char* component);
    void track_deallocation(void* ptr, const char* component);
    
    // Emergency handling
    void handle_allocation_failure(size_t size, const char* component);
    void handle_memory_corruption(void* ptr, const char* component);
}

// =============================================================================
// CONFIGURATION MACROS
// =============================================================================

/**
 * Note: We avoid redefining NimBLE's built-in configuration macros
 * to prevent compilation warnings. Instead, we configure through
 * platformio.ini build flags and runtime configuration.
 */

// Custom memory allocation hooks (not conflicting with NimBLE)
#define STATIC_BLE_MALLOC(size) NimBLEMemoryHooks::static_malloc(size)
#define STATIC_BLE_FREE(ptr) NimBLEMemoryHooks::static_free(ptr)
#define STATIC_BLE_REALLOC(ptr, size) NimBLEMemoryHooks::static_realloc(ptr, size)
#define STATIC_BLE_CALLOC(num, size) NimBLEMemoryHooks::static_calloc(num, size)

// =============================================================================
// INITIALIZATION HELPERS
// =============================================================================

/**
 * Helper functions to properly initialize NimBLE with static memory
 */

// Initialize NimBLE with static memory configuration
bool initializeNimBLEWithStaticMemory();

// Shutdown NimBLE and clean up static memory
void shutdownNimBLEStaticMemory();

// Validate NimBLE static memory configuration
bool validateNimBLEStaticMemory();

// Emergency recovery for NimBLE memory issues
bool recoverNimBLEStaticMemory();

// =============================================================================
// MONITORING AND DEBUGGING
// =============================================================================

/**
 * Production monitoring for NimBLE static memory usage
 */

struct NimBLEMemoryUsage {
    size_t totalAllocated;
    size_t peakUsage;
    uint32_t allocationCount;
    uint32_t failureCount;
    bool isHealthy;
    float efficiency;
};

// Get current NimBLE memory usage statistics
NimBLEMemoryUsage getNimBLEMemoryUsage();

// Log detailed NimBLE memory status
void logNimBLEMemoryStatus();

// Check if NimBLE memory is healthy
bool isNimBLEMemoryHealthy();

// =============================================================================
// ERROR HANDLING
// =============================================================================

/**
 * Error handling for NimBLE static memory issues
 */

enum class NimBLEMemoryError {
    ALLOCATION_FAILED,
    CORRUPTION_DETECTED,
    POOL_EXHAUSTED,
    INVALID_POINTER,
    CONFIGURATION_FAILED
};

// Error callback type
typedef void (*NimBLEMemoryErrorCallback)(NimBLEMemoryError error, const char* details);

// Set error callback for production monitoring
void setNimBLEMemoryErrorCallback(NimBLEMemoryErrorCallback callback);

// Handle NimBLE memory errors
void handleNimBLEMemoryError(NimBLEMemoryError error, const char* details);

#endif // NIMBLE_STATIC_CONFIG_H
