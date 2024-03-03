find_package(GTest REQUIRED)

file(GLOB_RECURSE sources
  ${CMAKE_CURRENT_SOURCE_DIR}/graphTests/GetOptimizedGraphTest.cpp
)

add_subdirectory(common)

add_executable(runTests main.cpp ${sources})

target_link_libraries(runTests PRIVATE
  CompilerLib
  GraphLib
  ParserLib
  PrinterLib
  ProgramLib
  ${GTEST_LIBRARIES}
  pthread
  GraphCreator
)
