cmake_minimum_required(VERSION 3.16)
project(GridPulseEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch pybind11 automatically
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# Collect all source files
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")

# Static library
add_library(gridpulse_engine STATIC ${ENGINE_SOURCES})
target_include_directories(gridpulse_engine PUBLIC include)

# CLI executable
add_executable(gridpulse_cli cli/main.cpp)
target_link_libraries(gridpulse_cli PRIVATE gridpulse_engine)

# Python bindings module
pybind11_add_module(gridpulse_engine_py bindings/pybind_module.cpp)
target_link_libraries(gridpulse_engine_py PRIVATE gridpulse_engine)

# Tests
enable_testing()