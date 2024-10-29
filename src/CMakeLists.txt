# Copyright (c) 2020 - 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
# NOTE: THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
# Main library
set(library_sources
    ${PROJECT_SOURCE_DIR}/src/erin.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_component.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_csv.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_distribution.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_graph.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_load.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_logging.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_lookup_table.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_random.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_reliability.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_scenario.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_simulation.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_simulation_info.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_time_and_amount.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_timestate.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_toml.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_units.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/erin_validation.cpp
    ${PROJECT_SOURCE_DIR}/include/erin/component.h
    ${PROJECT_SOURCE_DIR}/include/erin/const.h
    ${PROJECT_SOURCE_DIR}/include/erin/csv.h
    ${PROJECT_SOURCE_DIR}/include/erin/distribution.h
    ${PROJECT_SOURCE_DIR}/include/erin/erin.h
    ${PROJECT_SOURCE_DIR}/include/erin/graph.h
    ${PROJECT_SOURCE_DIR}/include/erin/load.h
    ${PROJECT_SOURCE_DIR}/include/erin/logging.h
    ${PROJECT_SOURCE_DIR}/include/erin/lookup_table.h
    ${PROJECT_SOURCE_DIR}/include/erin/random.h
    ${PROJECT_SOURCE_DIR}/include/erin/reliability.h
    ${PROJECT_SOURCE_DIR}/include/erin/result.h
    ${PROJECT_SOURCE_DIR}/include/erin/scenario.h
    ${PROJECT_SOURCE_DIR}/include/erin/simulation-info.h
    ${PROJECT_SOURCE_DIR}/include/erin/simulation.h
    ${PROJECT_SOURCE_DIR}/include/erin/time_and_amount.h
    ${PROJECT_SOURCE_DIR}/include/erin/timestate.h
    ${PROJECT_SOURCE_DIR}/include/erin/toml.h
    ${PROJECT_SOURCE_DIR}/include/erin/units.h
    ${PROJECT_SOURCE_DIR}/include/erin/utils.h
    ${PROJECT_SOURCE_DIR}/include/erin/valdata.h
    ${PROJECT_SOURCE_DIR}/include/erin/validation.h
    ${PROJECT_SOURCE_DIR}/include/erin/version.h
    ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/version.h
)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${library_sources})
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(${PROJECT_NAME} SHARED ${library_sources})
endif ()

target_include_directories(${PROJECT_NAME}
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC "${PROJECT_BINARY_DIR}/include"
    PRIVATE "${PROJECT_SOURCE_DIR}/src"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_build_options)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

# If MSVC_RUNTIME_LIBRARY is not by a parent project use the default.
# It's not clear why this is needed, since documentation indicates it
#   should be happening with the CMP0091 policy set to NEW.
get_target_property(RTL ${PROJECT_NAME} MSVC_RUNTIME_LIBRARY)
if ("${RTL}" STREQUAL "RTL-NOTFOUND")
    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif ()

if (${PROJECT_NAME}_COVERAGE)
    add_coverage(${PROJECT_NAME})
endif ()