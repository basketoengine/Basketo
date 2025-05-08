#pragma once

#include <string>

struct ScriptComponent {
    std::string scriptPath; // Path to the Lua script file
    // bool initialized = false; // To track if the script has been loaded and its init function called
    // Add any other script-related data here, e.g., a sol::table or sol::environment for this script instance
};
