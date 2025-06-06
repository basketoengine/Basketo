cmake_minimum_required(VERSION 3.10)

project(BasketoTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/googletest)

add_executable(BasketoTests
    test_game.cpp
    test_input.cpp
    test_asset.cpp
    test_memory.cpp
)

target_link_libraries(BasketoTests
    gtest
    gtest_main
    ${PROJECT_SOURCE_DIR}/src/Basketo
    SDL2
    SDL2main
)
