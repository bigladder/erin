# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
add_executable(test_erin_next
	test_erin_next.cpp)
target_include_directories(test_erin_next PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_erin_next erin_next)
