# Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
# See the LICENSE.txt file for additional terms and conditions.
set(test_sources
  erin_logging_tests.cpp
  erin_lookup_table_tests.cpp
  erin_random_tests.cpp
  erin_simulation_tests.cpp
  erin_switch_tests.cpp
  erin_tests.cpp
)

add_executable(${PROJECT_NAME}_tests ${test_sources})

target_link_libraries(${PROJECT_NAME}_tests
  PRIVATE ${PROJECT_NAME} gtest ${PROJECT_NAME}_build_options
  GTest::gtest_main
  toml11::toml11
  CLI11
  courier
)
target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)