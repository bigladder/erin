# Copyright (c) 2020 - 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
# NOTE: THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
configure_file(
	${PROJECT_SOURCE_DIR}/app/compilation_settings.h.in
	${PROJECT_BINARY_DIR}/app/compilation_settings.h
)

add_executable(erin
  erin.cpp
)
target_include_directories(erin
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PRIVATE ${PROJECT_BINARY_DIR}/app
)
target_link_libraries(erin
  ${PROJECT_NAME}lib
  toml11
  CLI11
  courier
)

add_executable(erin_stress_test
  erin_stress_test.cpp
)
target_include_directories(erin_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PRIVATE ${PROJECT_BINARY_DIR}/app
)
target_link_libraries(erin_stress_test
  ${PROJECT_NAME}lib
  toml11
  CLI11
  courier
)