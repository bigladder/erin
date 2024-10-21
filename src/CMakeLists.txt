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
include(../cmake/get-git-hash.cmake)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

option( ${CMAKE_PROJECT_NAME}_STATIC_LIB "Make a static library" ON )

if (${CMAKE_PROJECT_NAME}_COVERAGE)
  add_coverage(${CMAKE_PROJECT_NAME})
endif()

add_library(erinlib
  erin_logging.cpp
	erin.cpp
	erin_distribution.cpp
	erin_reliability.cpp
	erin_timestate.cpp
	erin_simulation_info.cpp
	erin_toml.cpp
	erin_load.cpp
	erin_time_and_amount.cpp
	erin_component.cpp
	erin_simulation.cpp
	erin_scenario.cpp
	erin_units.cpp
	erin_utils.cpp
	erin_csv.cpp
	erin_random.cpp
	erin_validation.cpp
	erin_graph.cpp
	erin_lookup_table.cpp
	"${PROJECT_SOURCE_DIR}/include/erin/logging.h"
	"${PROJECT_SOURCE_DIR}/include/erin/erin.h"
	"${PROJECT_SOURCE_DIR}/include/erin/valdata.h"
	"${PROJECT_SOURCE_DIR}/include/erin/distribution.h"
	"${PROJECT_SOURCE_DIR}/include/erin/reliability.h"
	"${PROJECT_SOURCE_DIR}/include/erin/timestate.h"
	"${PROJECT_SOURCE_DIR}/include/erin/simulation_info.h"
	"${PROJECT_SOURCE_DIR}/include/erin/toml.h"
	"${PROJECT_SOURCE_DIR}/include/erin/time_and_amount.h"
	"${PROJECT_SOURCE_DIR}/include/erin/load.h"
	"${PROJECT_SOURCE_DIR}/include/erin/validation.h"
	"${PROJECT_SOURCE_DIR}/include/erin/component.h"
  "${PROJECT_SOURCE_DIR}/include/erin/simulation.h"
	"${PROJECT_SOURCE_DIR}/include/erin/scenario.h"
	"${PROJECT_SOURCE_DIR}/include/erin/units.h"
	"${PROJECT_SOURCE_DIR}/include/erin/result.h"
	"${PROJECT_SOURCE_DIR}/include/erin/utils.h"
	"${PROJECT_SOURCE_DIR}/include/erin/csv.h"
	"${PROJECT_SOURCE_DIR}/include/erin/random.h"
	"${PROJECT_SOURCE_DIR}/include/erin/graph.h"
	"${PROJECT_SOURCE_DIR}/include/erin/lookup_table.h"
)

target_link_libraries(erinlib
  PRIVATE courier fmt
)

# Add Warnings
if(MSVC)
  target_compile_options(erinlib PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(erinlib PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
endif()
