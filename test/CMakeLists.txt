# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
enable_testing()

string(TOLOWER ${PROJECT_NAME} project_name_lower)

set(library_files
		erin_tests.cpp
		erin_next_random_test.cpp
		)

add_executable(${project_name_lower}_tests ${library_files})

target_include_directories(${project_name_lower}_tests PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${project_name_lower}_tests
	erin_next
	GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(${project_name_lower}_tests)
