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

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(BasketoGameEngine 
    src/main.cpp 
    src/Game.cpp 
    utils/utility.cpp 
    src/InputManager.cpp
    src/Physics.cpp
    src/SceneManager.cpp
    src/scenes/GameScene.cpp
    src/scenes/MenuScene.cpp
)

target_link_libraries(BasketoGameEngine 
    ${SDL2_LIBRARIES} 
    ${SDL2_IMAGE_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES}
)
