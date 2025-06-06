cmake_minimum_required(VERSION 3.10)
project(BasketoGameEngine)

# Use vcpkg
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

# Download and unpack googletest at configure time
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vendor/googletest/CMakeLists.txt"
              "${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add vcpkg's include directories to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/include)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)

# Provide a hint for Lua 5.4 include directory
set(LUA_INCLUDE_DIR /usr/include/lua5.4 CACHE PATH "Path to Lua 5.4 include directory")

# Find Lua development libraries (e.g., for Lua 5.4)
find_package(Lua 5.4 REQUIRED)

# Add Sol2 subdirectory
add_subdirectory(vendor/sol2)

add_subdirectory(src)
add_subdirectory(tests)

pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_IMAGE_LIBRARY_DIRS})

pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
include_directories(${SDL2_TTF_INCLUDE_DIRS})
link_directories(${SDL2_TTF_LIBRARY_DIRS})

pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
link_directories(${SDL2_MIXER_LIBRARY_DIRS})

# Add OpenGL (needed by some ImGui backends or for future use)
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})

include_directories(${SDL2_INCLUDE_DIRS})

# Add ImGui include directory
include_directories(vendor/imgui)
include_directories(vendor/libtinyfiledialogs)

set(SOURCE_FILES
    src/main.cpp
    src/Game.cpp
    src/InputManager.cpp
    src/SceneManager.cpp
    src/AssetManager.cpp
    src/scenes/GameScene.cpp
    src/scenes/MenuScene.cpp
    src/scenes/DevModeScene.cpp
    utils/utility.cpp
    src/Physics.cpp

    # Add ImGui core source files
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_sdl2.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
    vendor/libtinyfiledialogs/tinyfiledialogs.c
    src/ecs/systems/ScriptSystem.cpp
)

# Define the physics test executable
add_executable(PhysicsTest
    tests/test_physics.cpp
    src/Physics.cpp
    utils/utility.cpp
)

# Define the main game executable
add_executable(BasketoGameEngine ${SOURCE_FILES})

# Add Lua include directory to BasketoGameEngine
if(LUA_FOUND)
    target_include_directories(BasketoGameEngine PUBLIC ${LUA_INCLUDE_DIR})
endif()

# Link libraries for the main game
target_link_libraries(BasketoGameEngine
    PRIVATE # Default for libraries unless their headers are in public API
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${OpenGL_LIBRARIES}
    ${LUA_LIBRARIES} # Add Lua libraries
    PUBLIC sol2 # sol2 is an interface library, its include dirs need to be propagated
)

# Link libraries for the physics test
target_link_libraries(PhysicsTest
    ${SDL2_LIBRARIES}
)