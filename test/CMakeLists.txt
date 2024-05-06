# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
enable_testing()

string(TOLOWER ${PROJECT_NAME} project_name_lower)

# erin_tests
add_executable(${project_name_lower}_tests ${project_name_lower}_tests.cpp)

target_include_directories(${project_name_lower}_tests PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${project_name_lower}_tests
	erin_next
	GTest::gtest_main)

# erin_next_random_tests
add_executable(${project_name_lower}_next_random_tests ${project_name_lower}_next_random_tests.cpp erin_next_random_tests.cpp)

target_include_directories(${project_name_lower}_next_random_tests PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${project_name_lower}_next_random_tests
		erin_next
		GTest::gtest_main)

# lookup_table
add_executable(${project_name_lower}_lookup_table_tests
	${project_name_lower}_lookup_table_tests.cpp
	${PROJECT_SOURCE_DIR}/src/erin_next_lookup_table.cpp
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_lookup_table.h
)

target_include_directories(${project_name_lower}_lookup_table_tests
	PUBLIC
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${project_name_lower}_lookup_table_tests
	GTest::gtest_main
)

# Google Test
include(GoogleTest)

gtest_discover_tests(${project_name_lower}_tests)
gtest_discover_tests(${project_name_lower}_next_random_tests)
gtest_discover_tests(${project_name_lower}_lookup_table_tests)

