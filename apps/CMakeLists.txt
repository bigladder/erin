add_executable(test_erin_next
	test_erin_next.cpp)
target_include_directories(test_erin_next PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_erin_next erin_next)

add_executable(erin_next_stress_test
	erin_next_stress_test.cpp)
target_include_directories(erin_next_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erin_next_stress_test erin_next)

add_executable(erin_next_cli
	cli.cpp)
target_include_directories(erin_next_cli
	PUBLIC "${PROJECT_SOURCE_DIR}/include"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/CLI11")
target_link_libraries(erin_next_cli erin_next CLI11)

add_executable(erin_next_random_test
	erin_next_random_test.cpp)
target_include_directories(erin_next_random_test
	PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(erin_next_random_test erin_next)