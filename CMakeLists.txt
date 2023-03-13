cmake_minimum_required(VERSION 3.1)

project(RegularGamesCompiler CXX)

find_package(nlohmann_json 3.10.5 REQUIRED)
find_package(Boost 1.71.0 COMPONENTS program_options REQUIRED)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

# TODO fix it

# add_library(parser src/parser/parser.cpp)
# add_library(printer src/printer/printer.cpp)
# add_library(action src/graph/action.cpp)
# add_library(expression src/graph/expression.cpp)
# add_library(graph src/graph/graph.cpp)
# add_library(program src/program/program.cpp)
# add_library(compiler src/compiler/compiler.cpp)

# add_executable(rg2cpp src/main.cpp)

# target_link_libraries(rg2cpp PRIVATE nlohmann_json::nlohmann_json parser printer action expression graph program compiler)

add_executable(rg2cpp
    src/main.cpp
    src/parser/parser.cpp
    src/printer/printer.cpp
    src/graph/action.cpp
    src/graph/actionFactory.cpp
    src/graph/expression.cpp
    src/graph/expressionFactory.cpp
    src/graph/graph.cpp
    src/program/program.cpp
    src/compiler/valueAssigner.cpp
    src/compiler/containerChooser.cpp
    src/compiler/IContainer.cpp
    src/compiler/BitArrayContainer.cpp
    src/compiler/StlContainerBase.cpp
    src/compiler/SetContainer.cpp
    src/compiler/UnorderedSetContainer.cpp
    src/compiler/compiler.cpp)

target_link_libraries(rg2cpp Boost::program_options)
