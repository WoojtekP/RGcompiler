find_package(GTest REQUIRED)

file(GLOB_RECURSE sources
  ${CMAKE_CURRENT_SOURCE_DIR}/graphOperationsTests/GetOptimizedGraphOperatorTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphOperationsTests/GenerateGraphsOperatorTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graphOperationsTests/PragmaSimpleApplyOperatorTests.cpp
)

add_subdirectory(common)

add_executable(runTests main.cpp ${sources})

target_link_libraries(runTests PRIVATE
  GraphLib
  GraphOperationsLib
  ${GTEST_LIBRARIES}
  pthread
  GraphCreator
)
