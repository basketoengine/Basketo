#pragma once
#include <unordered_map>
#include <string>
#include <iostream>
#include <sstream>

class MemoryTracker {
public:
    static void* allocate(size_t size, const char* file, int line);
    static void deallocate(void* ptr);
    static void reportLeaks();
private:
    static std::unordered_map<void*, std::string> allocations;
};
