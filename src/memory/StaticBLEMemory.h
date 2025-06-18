#ifndef STATIC_BLE_MEMORY_H
#define STATIC_BLE_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// =============================================================================
// STATIC BLE MEMORY POOL CONFIGURATION
// =============================================================================

// Memory pool sizes (carefully calculated for NimBLE requirements)
#define BLE_STATIC_POOL_SIZE        8192    // 8KB total pool
#define BLE_CONNECTION_POOL_SIZE    2048    // 2KB for connection contexts
#define BLE_CHARACTERISTIC_POOL_SIZE 1024   // 1KB for characteristic buffers
#define BLE_CALLBACK_POOL_SIZE      512     // 512B for callback structures
#define BLE_EVENT_POOL_SIZE         1024    // 1KB for event queues
#define BLE_GENERAL_POOL_SIZE       3584    // Remaining for general BLE use

// Pool alignment requirements
#define BLE_MEMORY_ALIGNMENT        4       // 4-byte alignment for ESP32
#define BLE_MAX_ALLOCATIONS         64      // Maximum number of allocations

// Memory block header for tracking
struct BLEMemoryBlock {
    size_t size;
    bool allocated;
    uint32_t magic;     // For corruption detection
    void* next;         // For free list management
};

#define BLE_MEMORY_MAGIC 0xBEEFCAFE

// =============================================================================
// STATIC BLE MEMORY MANAGER CLASS
// =============================================================================

class StaticBLEMemory {
public:
    // Initialization and lifecycle
    static bool initialize();
    static void shutdown();
    static bool isInitialized();
    
    // Memory allocation interface
    static void* allocate(size_t size);
    static void deallocate(void* ptr);
    static bool reallocate(void* ptr, size_t new_size);
    
    // Pool-specific allocation (for optimization)
    static void* allocateConnection(size_t size);
    static void* allocateCharacteristic(size_t size);
    static void* allocateCallback(size_t size);
    static void* allocateEvent(size_t size);
    
    // Memory monitoring and debugging
    static size_t getTotalSize();
    static size_t getUsedSize();
    static size_t getFreeSize();
    static float getFragmentationRatio();
    static uint32_t getAllocationCount();
    static uint32_t getDeallocationCount();
    
    // Health monitoring
    static bool isHealthy();
    static void logMemoryStatus();
    static bool validateIntegrity();
    
    // Emergency operations
    static void emergencyReset();
    static bool compactMemory();
    
private:
    // Static memory pools (allocated at compile time)
    static uint8_t connectionPool[BLE_CONNECTION_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
    static uint8_t characteristicPool[BLE_CHARACTERISTIC_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
    static uint8_t callbackPool[BLE_CALLBACK_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
    static uint8_t eventPool[BLE_EVENT_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
    static uint8_t generalPool[BLE_GENERAL_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
    
    // Pool management structures
    struct PoolManager {
        uint8_t* pool;
        size_t size;
        size_t used;
        BLEMemoryBlock* freeList;
        uint32_t allocationCount;
        bool initialized;
    };
    
    static PoolManager pools[5];  // One for each pool type
    
    // Global state
    static bool initialized;
    static uint32_t totalAllocations;
    static uint32_t totalDeallocations;
    static uint32_t peakUsage;
    
    // Internal allocation methods
    static void* allocateFromPool(PoolManager* pool, size_t size);
    static bool deallocateFromPool(PoolManager* pool, void* ptr);
    static PoolManager* findPoolForPointer(void* ptr);
    
    // Pool initialization
    static bool initializePool(PoolManager* pool, uint8_t* memory, size_t size);
    static void resetPool(PoolManager* pool);
    
    // Memory management utilities
    static void* alignPointer(void* ptr);
    static size_t alignSize(size_t size);
    static bool isValidPointer(void* ptr);
    static bool isPointerInPool(PoolManager* pool, void* ptr);
    
    // Free list management
    static void addToFreeList(PoolManager* pool, BLEMemoryBlock* block);
    static BLEMemoryBlock* removeFromFreeList(PoolManager* pool, size_t size);
    static void coalesceFreeBlocks(PoolManager* pool);
    
    // Integrity checking
    static bool validatePool(PoolManager* pool);
    static bool validateBlock(BLEMemoryBlock* block);
    static void corruptionDetected(PoolManager* pool, void* ptr);
    
    // Pool type identification
    enum PoolType {
        POOL_CONNECTION = 0,
        POOL_CHARACTERISTIC = 1,
        POOL_CALLBACK = 2,
        POOL_EVENT = 3,
        POOL_GENERAL = 4
    };
    
    static PoolType getOptimalPool(size_t size);
    static const char* getPoolName(PoolType type);
};

// =============================================================================
// NIMBLE INTEGRATION HOOKS
// =============================================================================

// Custom allocator functions for NimBLE integration
extern "C" {
    void* ble_static_malloc(size_t size);
    void ble_static_free(void* ptr);
    void* ble_static_realloc(void* ptr, size_t size);
    void* ble_static_calloc(size_t num, size_t size);
}

// NimBLE memory configuration
void configure_nimble_static_memory();
void restore_nimble_default_memory();

// Memory monitoring for production
struct BLEMemoryStats {
    size_t totalSize;
    size_t usedSize;
    size_t freeSize;
    float fragmentationRatio;
    uint32_t allocationCount;
    uint32_t deallocationCount;
    uint32_t peakUsage;
    bool isHealthy;
    uint32_t corruptionCount;
};

BLEMemoryStats getBLEMemoryStats();

#endif // STATIC_BLE_MEMORY_H
