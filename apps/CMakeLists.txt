# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
add_executable(erin_next_stress_test
	erin_next_stress_test.cpp)
target_include_directories(erin_next_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erin_next_stress_test erin_next)

add_executable(erin_next_cli
	cli.cpp)
target_include_directories(erin_next_cli
	PUBLIC "${PROJECT_SOURCE_DIR}/include"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
target_link_libraries(erin_next_cli erin_next)

add_executable(erin_next_random_test
	erin_next_random_test.cpp)
target_include_directories(erin_next_random_test
	PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(erin_next_random_test erin_next)

add_executable(erin_next_graph
	graph.cpp)
target_include_directories(erin_next_graph
	PUBLIC "${PROJECT_SOURCE_DIR}/include"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
target_link_libraries(erin_next_graph erin_next)
