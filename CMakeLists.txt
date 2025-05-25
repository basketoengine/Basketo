cmake_minimum_required(VERSION 3.10)
project(BasketoGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)

set(LUA_INCLUDE_DIR /usr/include/lua5.4 CACHE PATH "Path to Lua 5.4 include directory")

find_package(Lua 5.4 REQUIRED)

add_subdirectory(vendor/sol2)

pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_IMAGE_LIBRARY_DIRS})

pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
include_directories(${SDL2_TTF_INCLUDE_DIRS})
link_directories(${SDL2_TTF_LIBRARY_DIRS})

pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
link_directories(${SDL2_MIXER_LIBRARY_DIRS})

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})

include_directories(${SDL2_INCLUDE_DIRS})

include_directories(vendor/imgui)
include_directories(vendor/libtinyfiledialogs)

add_executable(PhysicsTest
    tests/test_physics.cpp
    src/Physics.cpp
    src/spatial/Quadtree.cpp
    utils/utility.cpp
)

add_executable(BasketoGameEngine)

target_sources(BasketoGameEngine PRIVATE
    # Core application files
    src/main.cpp
    src/Game.cpp
    src/InputManager.cpp
    src/SceneManager.cpp
    src/AssetManager.cpp
    src/Physics.cpp
    src/spatial/Quadtree.cpp

    # Scenes
    src/scenes/GameScene.cpp
    src/scenes/MenuScene.cpp
    src/scenes/DevModeScene.cpp
    src/scenes/DevModeInputHandler.cpp
    src/scenes/DevModeSceneSerializer.cpp
    src/scenes/InspectorPanel.cpp

    # ECS Systems
    src/ecs/systems/AudioSystem.cpp
    src/ecs/systems/CameraSystem.cpp
    src/ecs/systems/ScriptSystem.cpp
    src/ecs/systems/AnimationSystem.cpp

    # Utilities
    utils/utility.cpp
    src/utils/EditorUtils.cpp
    src/utils/FileUtils.cpp
    src/utils/Console.cpp

    # Vendor libraries (compiled directly)
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_sdl2.cpp
    vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
    vendor/libtinyfiledialogs/tinyfiledialogs.c
)

if(LUA_FOUND)
    target_include_directories(BasketoGameEngine PUBLIC ${LUA_INCLUDE_DIR})
endif()

target_link_libraries(BasketoGameEngine
    PRIVATE 
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${OpenGL_LIBRARIES}
    ${LUA_LIBRARIES}
    PUBLIC sol2 
)

target_link_libraries(PhysicsTest
    ${SDL2_LIBRARIES}
)