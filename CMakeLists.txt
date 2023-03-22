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
    src/parser/Parser.cpp
    src/printer/Printer.cpp
    src/graph/Action.cpp
    src/graph/ActionFactory.cpp
    src/graph/Expression.cpp
    src/graph/ExpressionFactory.cpp
    src/graph/Graph.cpp
    src/program/Program.cpp
    src/compiler/ValueAssigner.cpp
    src/compiler/cacheContainers/ContainerChooser.cpp
    src/compiler/cacheContainers/BitArrayContainer.cpp
    src/compiler/cacheContainers/StlContainerBase.cpp
    src/compiler/cacheContainers/SetContainer.cpp
    src/compiler/cacheContainers/UnorderedSetContainer.cpp
    src/compiler/Compiler.cpp)

target_link_libraries(rg2cpp Boost::program_options)
