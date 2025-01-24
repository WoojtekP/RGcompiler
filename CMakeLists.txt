cmake_minimum_required(VERSION 3.1)

project(RegularGamesCompiler CXX)

find_package(nlohmann_json 3.10.5 REQUIRED)
find_package(Boost 1.71.0 COMPONENTS program_options REQUIRED)
find_package(GTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Release build flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Debug build flags
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

message(STATUS "CMake build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Default C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")

include_directories(src)
add_subdirectory(src)

if(GTest_FOUND)
    include_directories(unit-tests)
    add_subdirectory(unit-tests)
endif()
