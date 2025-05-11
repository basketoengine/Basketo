\
#include "Console.h"

namespace Console {
    void Log(const std::string& message) {
        std::cout << "[LOG] " << message << std::endl;
    }

    void Warn(const std::string& message) {
        std::cerr << "[WARN] " << message << std::endl;
    }

    void Error(const std::string& message) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}
