# Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
# See the LICENSE.txt file for additional terms and conditions.
# NOTE: THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
configure_file(
	${PROJECT_SOURCE_DIR}/app/compilation_settings.h.in
	${PROJECT_BINARY_DIR}/app/compilation_settings.h
)

add_executable(APP_erin
  erin.cpp
)
set_target_properties(APP_erin
  PROPERTIES OUTPUT_NAME erin
)
target_include_directories(APP_erin
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PRIVATE ${PROJECT_BINARY_DIR}/app
)
target_link_libraries(APP_erin
  ${PROJECT_NAME}
  toml11::toml11
  CLI11
  courier
)

add_executable(APP_erin_stress_test
  erin_stress_test.cpp
)
set_target_properties(APP_erin_stress_test
  PROPERTIES OUTPUT_NAME erin_stress_test
)
target_include_directories(APP_erin_stress_test
	PUBLIC ${PROJECT_SOURCE_DIR}/include
	PRIVATE ${PROJECT_BINARY_DIR}/app
)
target_link_libraries(APP_erin_stress_test
  ${PROJECT_NAME}
  toml11::toml11
  CLI11
  courier
)