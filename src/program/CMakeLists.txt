add_library(LoopFactoryLib
    LoopFactory.cpp
    LoopFactory.hpp
)

add_library(ProgramLib
    Program.cpp
    Program.hpp
)

target_link_libraries(ProgramLib
    PRIVATE
    ComparisonLib
)
