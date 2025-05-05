cmake_minimum_required(VERSION 3.10)
project(BasketoGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

find_package(PkgConfig REQUIRED)

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
)

add_executable(BasketoGameEngine ${SOURCE_FILES})

target_link_libraries(BasketoGameEngine
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${OpenGL_LIBRARIES}
)
