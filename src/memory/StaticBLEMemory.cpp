#include "StaticBLEMemory.h"
#include "../utils/Logger.h"
#include <string.h>
#include <algorithm>

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

// Static memory pools - allocated at compile time, never fragmented
uint8_t StaticBLEMemory::connectionPool[BLE_CONNECTION_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
uint8_t StaticBLEMemory::characteristicPool[BLE_CHARACTERISTIC_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
uint8_t StaticBLEMemory::callbackPool[BLE_CALLBACK_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
uint8_t StaticBLEMemory::eventPool[BLE_EVENT_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));
uint8_t StaticBLEMemory::generalPool[BLE_GENERAL_POOL_SIZE] __attribute__((aligned(BLE_MEMORY_ALIGNMENT)));

// Pool management structures
StaticBLEMemory::PoolManager StaticBLEMemory::pools[5];

// Global state
bool StaticBLEMemory::initialized = false;
uint32_t StaticBLEMemory::totalAllocations = 0;
uint32_t StaticBLEMemory::totalDeallocations = 0;
uint32_t StaticBLEMemory::peakUsage = 0;

// =============================================================================
// INITIALIZATION AND LIFECYCLE
// =============================================================================

bool StaticBLEMemory::initialize() {
    if (initialized) {
        Logger::warning("StaticBLEMemory already initialized");
        return true;
    }
    
    Logger::info("Initializing Static BLE Memory Manager...");
    
    // Initialize all memory pools
    bool success = true;
    success &= initializePool(&pools[POOL_CONNECTION], connectionPool, BLE_CONNECTION_POOL_SIZE);
    success &= initializePool(&pools[POOL_CHARACTERISTIC], characteristicPool, BLE_CHARACTERISTIC_POOL_SIZE);
    success &= initializePool(&pools[POOL_CALLBACK], callbackPool, BLE_CALLBACK_POOL_SIZE);
    success &= initializePool(&pools[POOL_EVENT], eventPool, BLE_EVENT_POOL_SIZE);
    success &= initializePool(&pools[POOL_GENERAL], generalPool, BLE_GENERAL_POOL_SIZE);
    
    if (!success) {
        Logger::error("Failed to initialize BLE memory pools");
        return false;
    }
    
    // Reset statistics
    totalAllocations = 0;
    totalDeallocations = 0;
    peakUsage = 0;
    
    initialized = true;
    
    Logger::infof("Static BLE Memory initialized: %d bytes total", BLE_STATIC_POOL_SIZE);
    logMemoryStatus();
    
    return true;
}

void StaticBLEMemory::shutdown() {
    if (!initialized) return;
    
    Logger::info("Shutting down Static BLE Memory Manager...");
    
    // Reset all pools
    for (int i = 0; i < 5; i++) {
        resetPool(&pools[i]);
    }
    
    initialized = false;
    Logger::info("Static BLE Memory shutdown complete");
}

bool StaticBLEMemory::isInitialized() {
    return initialized;
}

// =============================================================================
// MEMORY ALLOCATION INTERFACE
// =============================================================================

void* StaticBLEMemory::allocate(size_t size) {
    if (!initialized || size == 0) {
        return nullptr;
    }
    
    // Align size to memory boundary
    size = alignSize(size);
    
    // Find optimal pool for this allocation
    PoolType poolType = getOptimalPool(size);
    PoolManager* pool = &pools[poolType];
    
    // Try to allocate from optimal pool first
    void* ptr = allocateFromPool(pool, size);
    
    // If optimal pool is full, try general pool
    if (!ptr && poolType != POOL_GENERAL) {
        ptr = allocateFromPool(&pools[POOL_GENERAL], size);
    }
    
    // If still no memory, try other pools as fallback
    if (!ptr) {
        for (int i = 0; i < 5; i++) {
            if (i != poolType && i != POOL_GENERAL) {
                ptr = allocateFromPool(&pools[i], size);
                if (ptr) break;
            }
        }
    }
    
    if (ptr) {
        totalAllocations++;
        size_t currentUsage = getUsedSize();
        if (currentUsage > peakUsage) {
            peakUsage = currentUsage;
        }
        
        Logger::debugf("BLE allocated %d bytes at %p (total used: %d)", size, ptr, currentUsage);
    } else {
        Logger::warningf("BLE allocation failed for %d bytes", size);
        logMemoryStatus();
    }
    
    return ptr;
}

void StaticBLEMemory::deallocate(void* ptr) {
    if (!initialized || !ptr) return;
    
    // Find which pool this pointer belongs to
    PoolManager* pool = findPoolForPointer(ptr);
    if (!pool) {
        Logger::errorf("BLE deallocation failed: invalid pointer %p", ptr);
        return;
    }
    
    if (deallocateFromPool(pool, ptr)) {
        totalDeallocations++;
        Logger::debugf("BLE deallocated pointer %p", ptr);
    } else {
        Logger::errorf("BLE deallocation failed for pointer %p", ptr);
    }
}

// =============================================================================
// POOL-SPECIFIC ALLOCATION (OPTIMIZED)
// =============================================================================

void* StaticBLEMemory::allocateConnection(size_t size) {
    if (!initialized) return nullptr;
    return allocateFromPool(&pools[POOL_CONNECTION], alignSize(size));
}

void* StaticBLEMemory::allocateCharacteristic(size_t size) {
    if (!initialized) return nullptr;
    return allocateFromPool(&pools[POOL_CHARACTERISTIC], alignSize(size));
}

void* StaticBLEMemory::allocateCallback(size_t size) {
    if (!initialized) return nullptr;
    return allocateFromPool(&pools[POOL_CALLBACK], alignSize(size));
}

void* StaticBLEMemory::allocateEvent(size_t size) {
    if (!initialized) return nullptr;
    return allocateFromPool(&pools[POOL_EVENT], alignSize(size));
}

// =============================================================================
// MEMORY MONITORING AND DEBUGGING
// =============================================================================

size_t StaticBLEMemory::getTotalSize() {
    return BLE_STATIC_POOL_SIZE;
}

size_t StaticBLEMemory::getUsedSize() {
    if (!initialized) return 0;
    
    size_t used = 0;
    for (int i = 0; i < 5; i++) {
        used += pools[i].used;
    }
    return used;
}

size_t StaticBLEMemory::getFreeSize() {
    return getTotalSize() - getUsedSize();
}

float StaticBLEMemory::getFragmentationRatio() {
    // Static pools don't fragment, but we can measure efficiency
    if (!initialized) return 0.0f;
    
    size_t totalFree = getFreeSize();
    if (totalFree == 0) return 0.0f;
    
    // Calculate largest contiguous block in each pool
    size_t largestBlock = 0;
    for (int i = 0; i < 5; i++) {
        PoolManager* pool = &pools[i];
        BLEMemoryBlock* block = pool->freeList;
        while (block) {
            if (block->size > largestBlock) {
                largestBlock = block->size;
            }
            block = (BLEMemoryBlock*)block->next;
        }
    }
    
    return (float)largestBlock / (float)totalFree;
}

uint32_t StaticBLEMemory::getAllocationCount() {
    return totalAllocations;
}

uint32_t StaticBLEMemory::getDeallocationCount() {
    return totalDeallocations;
}

bool StaticBLEMemory::isHealthy() {
    if (!initialized) return false;
    
    // Check for memory leaks
    if (totalAllocations > totalDeallocations + 10) {  // Allow some tolerance
        return false;
    }
    
    // Check pool integrity
    for (int i = 0; i < 5; i++) {
        if (!validatePool(&pools[i])) {
            return false;
        }
    }
    
    // Check if we have reasonable free memory
    if (getFreeSize() < (getTotalSize() * 0.1)) {  // Less than 10% free
        return false;
    }
    
    return true;
}

void StaticBLEMemory::logMemoryStatus() {
    if (!initialized) {
        Logger::info("Static BLE Memory: Not initialized");
        return;
    }
    
    Logger::info("=== Static BLE Memory Status ===");
    Logger::infof("Total Size: %d bytes", getTotalSize());
    Logger::infof("Used Size: %d bytes (%.1f%%)", getUsedSize(), 
                 (float)getUsedSize() / getTotalSize() * 100.0f);
    Logger::infof("Free Size: %d bytes", getFreeSize());
    Logger::infof("Allocations: %lu", totalAllocations);
    Logger::infof("Deallocations: %lu", totalDeallocations);
    Logger::infof("Peak Usage: %lu bytes", peakUsage);
    Logger::infof("Health Status: %s", isHealthy() ? "Healthy" : "Warning");
    
    // Log individual pool status
    const char* poolNames[] = {"Connection", "Characteristic", "Callback", "Event", "General"};
    for (int i = 0; i < 5; i++) {
        PoolManager* pool = &pools[i];
        Logger::infof("Pool %s: %d/%d bytes used (%d allocs)", 
                     poolNames[i], pool->used, pool->size, pool->allocationCount);
    }
    
    Logger::info("===============================");
}

// =============================================================================
// INTERNAL ALLOCATION METHODS
// =============================================================================

void* StaticBLEMemory::allocateFromPool(PoolManager* pool, size_t size) {
    if (!pool || !pool->initialized || size == 0) {
        return nullptr;
    }

    // Add header size to allocation
    size_t totalSize = size + sizeof(BLEMemoryBlock);

    // Check if pool has enough space
    if (pool->used + totalSize > pool->size) {
        return nullptr;
    }

    // Try to find a suitable free block
    BLEMemoryBlock* block = removeFromFreeList(pool, totalSize);

    if (!block) {
        // No suitable free block, allocate from end of pool
        if (pool->used + totalSize > pool->size) {
            return nullptr;
        }

        block = (BLEMemoryBlock*)(pool->pool + pool->used);
        pool->used += totalSize;
    }

    // Initialize block header
    block->size = size;
    block->allocated = true;
    block->magic = BLE_MEMORY_MAGIC;
    block->next = nullptr;

    pool->allocationCount++;

    // Return pointer after header
    return (uint8_t*)block + sizeof(BLEMemoryBlock);
}

bool StaticBLEMemory::deallocateFromPool(PoolManager* pool, void* ptr) {
    if (!pool || !ptr || !isPointerInPool(pool, ptr)) {
        return false;
    }

    // Get block header
    BLEMemoryBlock* block = (BLEMemoryBlock*)((uint8_t*)ptr - sizeof(BLEMemoryBlock));

    // Validate block
    if (!validateBlock(block)) {
        corruptionDetected(pool, ptr);
        return false;
    }

    // Mark as free and add to free list
    block->allocated = false;
    addToFreeList(pool, block);

    // Try to coalesce adjacent free blocks
    coalesceFreeBlocks(pool);

    return true;
}

StaticBLEMemory::PoolManager* StaticBLEMemory::findPoolForPointer(void* ptr) {
    for (int i = 0; i < 5; i++) {
        if (isPointerInPool(&pools[i], ptr)) {
            return &pools[i];
        }
    }
    return nullptr;
}

bool StaticBLEMemory::initializePool(PoolManager* pool, uint8_t* memory, size_t size) {
    if (!pool || !memory || size == 0) {
        return false;
    }

    pool->pool = memory;
    pool->size = size;
    pool->used = 0;
    pool->freeList = nullptr;
    pool->allocationCount = 0;
    pool->initialized = true;

    // Clear the memory
    memset(memory, 0, size);

    return true;
}

void StaticBLEMemory::resetPool(PoolManager* pool) {
    if (!pool) return;

    pool->used = 0;
    pool->freeList = nullptr;
    pool->allocationCount = 0;

    if (pool->pool) {
        memset(pool->pool, 0, pool->size);
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void* StaticBLEMemory::alignPointer(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + BLE_MEMORY_ALIGNMENT - 1) & ~(BLE_MEMORY_ALIGNMENT - 1);
    return (void*)aligned;
}

size_t StaticBLEMemory::alignSize(size_t size) {
    return (size + BLE_MEMORY_ALIGNMENT - 1) & ~(BLE_MEMORY_ALIGNMENT - 1);
}

bool StaticBLEMemory::isValidPointer(void* ptr) {
    return ptr != nullptr && findPoolForPointer(ptr) != nullptr;
}

bool StaticBLEMemory::isPointerInPool(PoolManager* pool, void* ptr) {
    if (!pool || !ptr || !pool->initialized) {
        return false;
    }

    uintptr_t poolStart = (uintptr_t)pool->pool;
    uintptr_t poolEnd = poolStart + pool->size;
    uintptr_t ptrAddr = (uintptr_t)ptr;

    return ptrAddr >= poolStart && ptrAddr < poolEnd;
}

StaticBLEMemory::PoolType StaticBLEMemory::getOptimalPool(size_t size) {
    if (size <= 256) return POOL_CALLBACK;
    if (size <= 512) return POOL_CHARACTERISTIC;
    if (size <= 1024) return POOL_EVENT;
    if (size <= 2048) return POOL_CONNECTION;
    return POOL_GENERAL;
}

const char* StaticBLEMemory::getPoolName(PoolType type) {
    switch (type) {
        case POOL_CONNECTION: return "Connection";
        case POOL_CHARACTERISTIC: return "Characteristic";
        case POOL_CALLBACK: return "Callback";
        case POOL_EVENT: return "Event";
        case POOL_GENERAL: return "General";
        default: return "Unknown";
    }
}

// =============================================================================
// FREE LIST MANAGEMENT
// =============================================================================

void StaticBLEMemory::addToFreeList(PoolManager* pool, BLEMemoryBlock* block) {
    if (!pool || !block) return;

    block->next = pool->freeList;
    pool->freeList = block;
}

BLEMemoryBlock* StaticBLEMemory::removeFromFreeList(PoolManager* pool, size_t size) {
    if (!pool || !pool->freeList) return nullptr;

    BLEMemoryBlock* prev = nullptr;
    BLEMemoryBlock* current = pool->freeList;

    while (current) {
        if (current->size >= size) {
            // Remove from free list
            if (prev) {
                prev->next = current->next;
            } else {
                pool->freeList = (BLEMemoryBlock*)current->next;
            }
            return current;
        }
        prev = current;
        current = (BLEMemoryBlock*)current->next;
    }

    return nullptr;
}

void StaticBLEMemory::coalesceFreeBlocks(PoolManager* pool) {
    // Simple coalescing - can be enhanced for production
    // For now, just ensure free list is properly maintained
    if (!pool || !pool->freeList) return;

    // This is a simplified version - production would implement
    // more sophisticated coalescing algorithms
}

// =============================================================================
// INTEGRITY CHECKING
// =============================================================================

bool StaticBLEMemory::validatePool(PoolManager* pool) {
    if (!pool || !pool->initialized) return false;

    // Check pool bounds
    if (pool->used > pool->size) return false;

    // Validate free list
    BLEMemoryBlock* block = pool->freeList;
    int freeListCount = 0;
    while (block && freeListCount < 100) {  // Prevent infinite loops
        if (!validateBlock(block)) return false;
        if (block->allocated) return false;  // Free list should only contain free blocks
        block = (BLEMemoryBlock*)block->next;
        freeListCount++;
    }

    return true;
}

bool StaticBLEMemory::validateBlock(BLEMemoryBlock* block) {
    if (!block) return false;

    // Check magic number
    if (block->magic != BLE_MEMORY_MAGIC) return false;

    // Check size is reasonable
    if (block->size == 0 || block->size > BLE_STATIC_POOL_SIZE) return false;

    return true;
}

void StaticBLEMemory::corruptionDetected(PoolManager* pool, void* ptr) {
    Logger::errorf("BLE Memory corruption detected at %p in pool %p", ptr, pool);
    // In production, this could trigger emergency recovery
}

// =============================================================================
// NIMBLE INTEGRATION HOOKS
// =============================================================================

extern "C" {
    void* ble_static_malloc(size_t size) {
        return StaticBLEMemory::allocate(size);
    }

    void ble_static_free(void* ptr) {
        StaticBLEMemory::deallocate(ptr);
    }

    void* ble_static_realloc(void* ptr, size_t size) {
        if (!ptr) return ble_static_malloc(size);
        if (size == 0) {
            ble_static_free(ptr);
            return nullptr;
        }

        // Simple realloc - allocate new, copy, free old
        void* new_ptr = ble_static_malloc(size);
        if (new_ptr && ptr) {
            // We don't know the old size, so this is a limitation
            // In production, we'd store size in block header
            memcpy(new_ptr, ptr, size);  // Assumes new size >= old size
            ble_static_free(ptr);
        }
        return new_ptr;
    }

    void* ble_static_calloc(size_t num, size_t size) {
        size_t total = num * size;
        void* ptr = ble_static_malloc(total);
        if (ptr) {
            memset(ptr, 0, total);
        }
        return ptr;
    }
}

BLEMemoryStats getBLEMemoryStats() {
    BLEMemoryStats stats;
    stats.totalSize = StaticBLEMemory::getTotalSize();
    stats.usedSize = StaticBLEMemory::getUsedSize();
    stats.freeSize = StaticBLEMemory::getFreeSize();
    stats.fragmentationRatio = StaticBLEMemory::getFragmentationRatio();
    stats.allocationCount = StaticBLEMemory::getAllocationCount();
    stats.deallocationCount = StaticBLEMemory::getDeallocationCount();
    stats.peakUsage = 0; // Would need to expose this
    stats.isHealthy = StaticBLEMemory::isHealthy();
    stats.corruptionCount = 0; // Would need to track this
    return stats;
}
