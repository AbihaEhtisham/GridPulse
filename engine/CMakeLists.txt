cmake_minimum_required(VERSION 3.16)
project(GridPulseEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect all source files automatically
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")

# Static library
add_library(gridpulse_engine STATIC ${ENGINE_SOURCES})
target_include_directories(gridpulse_engine PUBLIC include)

# CLI executable
add_executable(gridpulse_cli cli/main.cpp)
target_link_libraries(gridpulse_cli PRIVATE gridpulse_engine)

# Tests
enable_testing()