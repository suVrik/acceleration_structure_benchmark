cmake_minimum_required(VERSION 3.19)

project(acceleration_structure_benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

file(GLOB_RECURSE SOURCES "source/*.cpp" "source/*.h")
add_executable(acceleration_structure_benchmark ${SOURCES})
