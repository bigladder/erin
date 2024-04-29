# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
configure_file(
	${PROJECT_SOURCE_DIR}/apps/compilation_settings.h.in
	${PROJECT_SOURCE_DIR}/apps/compilation_settings.h)

add_executable(erin_next_stress_test
	erin_next_stress_test.cpp)
target_include_directories(erin_next_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(erin_next_stress_test erin_next)

add_executable(erin erin.cpp compilation_settings.h.in)
target_include_directories(erin
	PUBLIC "${PROJECT_SOURCE_DIR}/include"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/CLI11")
target_link_libraries(erin erin_next CLI11)
