# Copyright (c) 2020-2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
# Main library
# Note from https://cmake.org/cmake/help/latest/command/file.html
# Note: We do not recommend using GLOB to collect a list of source files from
#       your source tree. If no CMakeLists.txt file changes when a source is
#       added or removed then the generated build system cannot know when to
#       ask CMake to regenerate. The CONFIGURE_DEPENDS flag may not work
#       reliably on all generators, or if a new generator is added in the
#       future that cannot support it, projects using it will be stuck. Even if
#       CONFIGURE_DEPENDS works reliably, there is still a cost to perform the
#       check on every rebuild. 
#file(GLOB_RECURSE sources "${PROJECT_SOURCE_DIR}/src/[a-zA-Z]*.cpp")
#file(GLOB_RECURSE public_headers "${PROJECT_SOURCE_DIR}/include/${CMAKE_PROJECT_NAME}/[a-zA-Z]*.h")
#file(GLOB_RECURSE private_headers "${PROJECT_SOURCE_DIR}/src/[a-zA-Z]*.h")
configure_file(
  ${PROJECT_SOURCE_DIR}/include/erin/version.h.in
  ${PROJECT_SOURCE_DIR}/include/erin/version.h)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

option( ${CMAKE_PROJECT_NAME}_STATIC_LIB "Make a static library" ON )

if (${CMAKE_PROJECT_NAME}_COVERAGE)
  add_coverage(${CMAKE_PROJECT_NAME})
endif()

add_library(erin_next
	erin_next.cpp
	erin_next_distribution.cpp
	erin_next_reliability.cpp
	erin_next_timestate.cpp
	erin_next_simulation_info.cpp
	erin_next_toml.cpp
	erin_next_load.cpp
	erin_next_time_and_amount.cpp
	erin_next_component.cpp
	erin_next_simulation.cpp
	erin_next_scenario.cpp
	erin_next_units.cpp
	erin_next_utils.cpp
	erin_next_csv.cpp
	erin_next_random.cpp
	erin_next_validation.cpp
	erin_next_graph.cpp
	erin_next_lookup_table.cpp
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_valdata.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_distribution.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_reliability.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_timestate.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_simulation_info.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_toml.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_time_and_amount.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_load.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_validation.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_component.h"
        "${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_simulation.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_scenario.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_units.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_result.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_utils.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_csv.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_random.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_graph.h"
	"${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_lookup_table.h"
)

target_link_libraries(erin_next PRIVATE fmt)

# Add Warnings
if(MSVC)
  target_compile_options(erin_next PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(erin_next PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
endif()
