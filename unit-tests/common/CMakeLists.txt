add_library(GraphCreator
    GraphCreator.cpp
    GraphCreator.hpp
)

target_link_libraries(GraphCreator PRIVATE
  ParserLib
  ValueAssignerLib
  LoopFactoryLib
  ProgramLib
  GraphLib
)
