# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
enable_testing()

# erin_tests
add_executable(erin_tests
	erin_tests.cpp)

target_include_directories(erin_tests
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(erin_tests
	erin_next
	GTest::gtest_main)

# erin_next_random_tests
add_executable(erin_next_random_tests
	erin_next_random_tests.cpp erin_next_random_tests.cpp)

target_include_directories(erin_next_random_tests
	PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(erin_next_random_tests
		erin_next
		GTest::gtest_main)

# lookup_table
add_executable(erin_lookup_table_tests
	erin_lookup_table_tests.cpp
	${PROJECT_SOURCE_DIR}/src/erin_next_lookup_table.cpp
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_lookup_table.h
)

target_include_directories(erin_lookup_table_tests
	PUBLIC
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(erin_lookup_table_tests
	erin_next
	GTest::gtest_main
)

# switch
add_executable(erin_switch_tests
	erin_switch_tests.cpp
)

target_include_directories(erin_switch_tests
	PUBLIC
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(erin_switch_tests
	erin_next
	GTest::gtest_main
)

# Google Test
include(GoogleTest)

gtest_discover_tests(erin_tests)
gtest_discover_tests(erin_next_random_tests)
gtest_discover_tests(erin_lookup_table_tests)
gtest_discover_tests(erin_switch_tests)

