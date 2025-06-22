add_library(CompilerLib
    Compiler.cpp
    Compiler.hpp
)

add_library(IntegerOperationsDeducerLib
    IntegerOperationsDeducer.cpp
    IntegerOperationsDeducer.hpp
)

add_library(SymbolsManagerLib
    SymbolsManager.cpp
    SymbolsManager.hpp
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

target_link_libraries(SymbolsManagerLib
    PRIVATE
    ParserLib
    IntegerOperationsDeducerLib
    ValueAssignerLib
    ValueFactoryLib
)

target_link_libraries(CompilerLib
    PRIVATE
    ComparisonLib
    GraphOperationsLib
    CacheContainersLib
    PragmaLib
    StateCacheLib
    SymbolsManagerLib
    IntegerOperationsDeducerLib
    ValueAssignerLib
    ValueFactoryLib
)
