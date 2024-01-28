add_library(CompilerLib
    Compiler.cpp
    Compiler.hpp
    ValueAssigner.cpp
    ValueAssigner.hpp
)

add_subdirectory(cacheContainers)
add_subdirectory(graphOperations)

target_link_libraries(CompilerLib
    PRIVATE
    GraphOperationsLib
    CacheContainersLib
)
