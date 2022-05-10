cmake_minimum_required(VERSION 3.1)

project(RegularGamesCompiler CXX)

find_package(nlohmann_json 3.10.5 REQUIRED)

add_executable(rg2cpp src/main.cpp)

target_link_libraries(rg2cpp PRIVATE nlohmann_json::nlohmann_json)
