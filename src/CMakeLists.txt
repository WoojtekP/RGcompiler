add_executable(rg2cpp main.cpp)

add_subdirectory(compiler)
add_subdirectory(graph)
add_subdirectory(parser)
add_subdirectory(printer)
add_subdirectory(program)

target_link_libraries(rg2cpp
    PRIVATE
    CompilerLib
    GraphLib
    ParserLib
    PrinterLib
    LoopFactoryLib
    ProgramLib
    nlohmann_json::nlohmann_json
    Boost::program_options
)
