# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
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

set(sources
  ${PROJECT_SOURCE_DIR}/src/erin.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_component.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_csv.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_converter.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_flow_limits.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_flow_meter.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_load.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_mover.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_mux.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_on_off_switch.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_storage.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_supply.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_devs_uncontrolled_source.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_distribution.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_element.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_fragility.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_generics.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_graphviz.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_input_reader.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_network.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_port.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_random.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_reliability.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_scenario.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_scenario_stats.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_scenario_results.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_stream.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_type.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_utils.cpp
  ${PROJECT_SOURCE_DIR}/src/toml_helper.cpp
)

set(public_headers
  ${PROJECT_SOURCE_DIR}/include/erin/component.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/converter.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/flow_limits.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/flow_meter.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/load.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/mover.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/mux.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/on_off_switch.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/storage.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/supply.h
  ${PROJECT_SOURCE_DIR}/include/erin/devs/uncontrolled_source.h
  ${PROJECT_SOURCE_DIR}/include/erin/distribution.h
  ${PROJECT_SOURCE_DIR}/include/erin/element.h
  ${PROJECT_SOURCE_DIR}/include/erin/erin.h
  ${PROJECT_SOURCE_DIR}/include/erin/fragility.h
  ${PROJECT_SOURCE_DIR}/include/erin/graphviz.h
  ${PROJECT_SOURCE_DIR}/include/erin/input_reader.h
  ${PROJECT_SOURCE_DIR}/include/erin/network.h
  ${PROJECT_SOURCE_DIR}/include/erin/port.h
  ${PROJECT_SOURCE_DIR}/include/erin/random.h
  ${PROJECT_SOURCE_DIR}/include/erin/reliability.h
  ${PROJECT_SOURCE_DIR}/include/erin/scenario.h
  ${PROJECT_SOURCE_DIR}/include/erin/scenario_stats.h
  ${PROJECT_SOURCE_DIR}/include/erin/scenario_results.h
  ${PROJECT_SOURCE_DIR}/include/erin/stream.h
  ${PROJECT_SOURCE_DIR}/include/erin/type.h
  ${PROJECT_SOURCE_DIR}/include/erin/utils.h
  ${PROJECT_SOURCE_DIR}/include/erin/version.h
)

set(private_headers
  ${PROJECT_SOURCE_DIR}/src/debug_utils.h
  ${PROJECT_SOURCE_DIR}/src/erin_csv.h
  ${PROJECT_SOURCE_DIR}/src/erin_generics.h
  ${PROJECT_SOURCE_DIR}/src/toml_helper.h
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/gsl
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/gsl_algorithm
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/gsl_assert
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/gsl_byte
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/gsl_util
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/multi_span
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/pointers
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/span
  ${PROJECT_SOURCE_DIR}/vendor/gsl/include/gsl/string_span
  ${PROJECT_SOURCE_DIR}/vendor/bdevs/include/adevs.h
  ${PROJECT_SOURCE_DIR}/vendor/bdevs/include/adevs_base.h
  ${PROJECT_SOURCE_DIR}/vendor/bdevs/include/adevs_pdevs.h
  ${PROJECT_SOURCE_DIR}/vendor/bdevs/include/adevs_time.h
)

set(library_sources
  ${sources}
  ${public_headers}
  ${private_headers}
)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

option( ${CMAKE_PROJECT_NAME}_STATIC_LIB "Make a static library" ON )

if (${CMAKE_PROJECT_NAME}_STATIC_LIB)
  add_library(${CMAKE_PROJECT_NAME} STATIC ${library_sources})
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${CMAKE_PROJECT_NAME}_STATIC_DEFINE")
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include")
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include")
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
else()
  set(CMAKE_MACOSX_RPATH 1)
  add_library(${CMAKE_PROJECT_NAME} SHARED ${library_sources})
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include")
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include")
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
endif()

include(GenerateExportHeader)
generate_export_header(${CMAKE_PROJECT_NAME})

# target_link_libraries(${CMAKE_PROJECT_NAME} vendor dependencies)

if (${CMAKE_PROJECT_NAME}_COVERAGE)
  add_coverage(${CMAKE_PROJECT_NAME})
endif()

add_executable(erin_distribution_test
  "${PROJECT_SOURCE_DIR}/src/cli_test_dist.cpp"
  "${PROJECT_SOURCE_DIR}/src/erin_csv.cpp"
  "${PROJECT_SOURCE_DIR}/src/erin_distribution.cpp"
  "${PROJECT_SOURCE_DIR}/src/erin_type.cpp"
  "${PROJECT_SOURCE_DIR}/src/erin_utils.cpp"
  "${PROJECT_SOURCE_DIR}/src/debug_utils.h"
  "${PROJECT_SOURCE_DIR}/src/erin_csv.h"
  "${PROJECT_SOURCE_DIR}/include/erin/distribution.h"
  "${PROJECT_SOURCE_DIR}/include/erin/type.h"
  "${PROJECT_SOURCE_DIR}/include/erin/utils.h"
  "${PROJECT_SOURCE_DIR}/include/erin/version.h"
  )
target_include_directories(erin_distribution_test
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include")
add_executable(erin_csv_test erin_csv_tester.cpp erin_csv.cpp erin_csv.h)
target_include_directories(erin_csv_test
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include")
add_executable(erin_single cli.cpp ${sources} ${public_headers} ${private_headers})
set_target_properties(erin_single
  PROPERTIES OUTPUT_NAME "erin") 
target_include_directories(erin_single
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
add_executable(erin_multi cli_multi.cpp ${sources} ${public_headers} ${private_headers})
target_include_directories(erin_multi
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
add_executable(erin_graph cli_graph.cpp ${sources} ${public_headers} ${private_headers})
target_include_directories(erin_graph
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
add_executable(str8760 stress_test_8760.cpp ${sources} ${public_headers} ${private_headers})
target_include_directories(str8760
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
add_executable(str8760x5000 stress_test_8760x5000.cpp ${sources} ${public_headers} ${private_headers})
target_include_directories(str8760x5000
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/gsl/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/bdevs/include"
  PRIVATE "${PROJECT_SOURCE_DIR}/vendor/toml11")
add_executable(limits limits.cpp)

add_library(erin_next
	erin_next.cpp
	erin_next_distribution.cpp
	erin_next_reliability.cpp
	erin_next_timestate.cpp
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next.h
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_distribution.h
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_reliability.h
	${PROJECT_SOURCE_DIR}/include/erin_next/erin_next_timestate.h
)

# Add Warnings
if(MSVC)
  target_compile_options(erin_single PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(erin_multi PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(erin_graph PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(str8760 PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(str8760x5000 PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(limits PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(erin_next PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(erin_single PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(erin_multi PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(erin_graph PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(str8760 PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(str8760x5000 PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(limits PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(erin_next PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:Release>:-O3>")
endif()
