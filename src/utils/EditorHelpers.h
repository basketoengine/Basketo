#pragma once
#include <cstring>
#include <string>
#include <algorithm>
#include <type_traits>

inline void copyToBuffer(const std::string& src, char* dest, size_t destSize) {
    std::strncpy(dest, src.c_str(), destSize - 1);
    dest[destSize - 1] = '\0';
}

template<typename T, typename ComponentManagerType, typename EntityType, typename... Args>
bool addComponentIfMissing(ComponentManagerType* cm, EntityType entity, Args&&... args) {
    if (!cm->template hasComponent<T>(entity)) {
        cm->template addComponent<T>(entity, T{std::forward<Args>(args)...});
        return true;
    }
    return false;
}
