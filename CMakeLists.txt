cmake_minimum_required(VERSION 3.10)
project(BasketoGameEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# Manually specify SDL2_image paths
include_directories(${SDL2_INCLUDE_DIRS} /usr/include/SDL2)
set(SDL2_IMAGE_LIBRARIES -lSDL2_image)

add_executable(BasketoGameEngine src/main.cpp src/Game.cpp utils/utility.cpp src/InputManager.cpp)

target_link_libraries(BasketoGameEngine ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})