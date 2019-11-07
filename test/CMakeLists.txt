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

add_executable(${CMAKE_PROJECT_NAME}_tests ${test_sources})

include(GoogleTest)
gtest_discover_tests(${CMAKE_PROJECT_NAME}_tests)
