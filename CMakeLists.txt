cmake_minimum_required(VERSION 3.1)

project(RegularGamesCompiler CXX)

find_package(nlohmann_json 3.10.5 REQUIRED)

include_directories(src)

add_library(parser src/parser/parser.cpp)
add_library(printer src/printer/printer.cpp)

add_executable(rg2cpp src/main.cpp)

target_link_libraries(rg2cpp PRIVATE nlohmann_json::nlohmann_json parser printer)
