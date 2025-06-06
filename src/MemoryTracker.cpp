#include "MemoryTracker.h"
#include <iostream>
#include <sstream>

std::unordered_map<void*, std::string> MemoryTracker::allocations;

void* MemoryTracker::allocate(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    std::stringstream ss;
    ss << file << ":" << line;
    allocations[ptr] = ss.str();
    return ptr;
}

void MemoryTracker::deallocate(void* ptr) {
    if (allocations.find(ptr) != allocations.end()) {
        allocations.erase(ptr);
    }
    free(ptr);
}

void MemoryTracker::reportLeaks() {
    if (allocations.empty()) return;
    
    std::cout << "Memory Leaks Detected:\n";
    for (const auto& alloc : allocations) {
        std::cout << "Leaked at " << alloc.second << "\n";
    }
}
