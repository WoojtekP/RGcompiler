add_library(CompilerLib
    Compiler.cpp
    Compiler.hpp
)

add_library(ValueAssignerLib
    ValueAssigner.cpp
    ValueAssigner.hpp
)

add_library(ValueFactoryLib
    ValueFactory.cpp
    ValueFactory.hpp
)

add_subdirectory(cacheContainers)
add_subdirectory(graphOperations)
add_subdirectory(pragma)
add_subdirectory(stateCache)

target_link_libraries(CompilerLib
    PRIVATE
    GraphOperationsLib
    CacheContainersLib
    PragmaLib
    StateCacheLib
    ValueAssignerLib
    ValueFactoryLib
)
