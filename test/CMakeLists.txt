include_directories("${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/include"
  "${gtest_SOURCE_DIR}/include"
  "checkout_line")

set(test_sources
  erin_tests.cpp
  erin_test_utils.h
  checkout_line/clerk.cpp
  checkout_line/clerk.h
  checkout_line/customer.h
  checkout_line/generator.cpp
  checkout_line/generator.h
  checkout_line/observer.cpp
  checkout_line/observer.h)

link_libraries(${CMAKE_PROJECT_NAME} gtest)

# Copy required ex02.csv to the build/test directory
# for use in testing.
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex02.csv"
  "ex02.csv"
  COPYONLY)
# The placement below is required for Visual Studio to find ex02.csv when it
# runs unit tests from the IDE.
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex02.csv"
  "${CMAKE_BINARY_DIR}/bin/ex02.csv"
  COPYONLY)
add_executable(${CMAKE_PROJECT_NAME}_tests ${test_sources})
target_include_directories(${CMAKE_PROJECT_NAME}_tests PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include")

include(GoogleTest)
gtest_discover_tests(${CMAKE_PROJECT_NAME}_tests)
