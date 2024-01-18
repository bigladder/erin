# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
cmake_minimum_required(VERSION 3.15..3.28) # required for gtest_discover_tests

project(ERIN
    VERSION 0.0.55
    DESCRIPTION "Energy Resilience of Interacting Networks"
    LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "CMAKE_BUILD_TYPE is " ${CMAKE_BUILD_TYPE})
set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option( ${CMAKE_PROJECT_NAME}_TESTING "Build ERIN testing targets" OFF)
option( ${CMAKE_PROJECT_NAME}_COVERAGE "Generate coverage reports" OFF)
option(BUILD_DOC "Build documentation" OFF)

# Set up testing/coverage
if (${CMAKE_PROJECT_NAME}_TESTING)
  include( CTest )
  enable_testing()
  if (${CMAKE_PROJECT_NAME}_COVERAGE)
    set(ENABLE_COVERAGE ON CACHE BOOL "" FORCE)
    find_package(codecov)
  endif()
endif()

# Add subdirectories
add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(apps)

if (${CMAKE_PROJECT_NAME}_TESTING)
  add_subdirectory(test)
  if (${CMAKE_PROJECT_NAME}_COVERAGE)
    coverage_evaluate()
  endif()
endif()

if (BUILD_DOC)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target(doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  else (DOXYGEN_FOUND)
    message("Doxygen needs to be installed to generate the doxygen documentation")
  endif (DOXYGEN_FOUND)
endif (BUILD_DOC)

# Copy input files to the build directory
# for use in regression testing (see scripts/doit).
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex01.toml"
  "ex01.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex02.toml"
  "ex02.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex02.csv"
  "ex02.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex03.toml"
  "ex03.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex03.csv"
  "ex03.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex04.toml"
  "ex04.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex05.toml"
  "ex05.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06.toml"
  "ex06.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-emergency-electric-1.csv"
  "ex06-emergency-electric-1.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-emergency-electric-2.csv"
  "ex06-emergency-electric-2.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-emergency-heating-1.csv"
  "ex06-emergency-heating-1.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-emergency-heating-2.csv"
  "ex06-emergency-heating-2.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-normal-electric-1.csv"
  "ex06-normal-electric-1.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-normal-electric-2.csv"
  "ex06-normal-electric-2.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-normal-heating-1.csv"
  "ex06-normal-heating-1.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex06-normal-heating-2.csv"
  "ex06-normal-heating-2.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-electric-mcc.csv"
  "ex07-emergency-electric-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-electric-oc.csv"
  "ex07-emergency-electric-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-heating-mcc.csv"
  "ex07-emergency-heating-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-heating-oc.csv"
  "ex07-emergency-heating-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-steam-mcc.csv"
  "ex07-emergency-steam-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-emergency-steam-oc.csv"
  "ex07-emergency-steam-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-electric-mcc.csv"
  "ex07-normal-electric-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-electric-oc.csv"
  "ex07-normal-electric-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-heating-mcc.csv"
  "ex07-normal-heating-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-heating-oc.csv"
  "ex07-normal-heating-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-steam-mcc.csv"
  "ex07-normal-steam-mcc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07-normal-steam-oc.csv"
  "ex07-normal-steam-oc.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex07.toml"
  "ex07.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex08.toml"
  "ex08.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex09.toml"
  "ex09.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex10.toml"
  "ex10.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex11.toml"
  "ex11.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex12.toml"
  "ex12.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex13.toml"
  "ex13.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex14.toml"
  "ex14.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex15.toml"
  "ex15.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex16.toml"
  "ex16.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex17.toml"
  "ex17.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex18.toml"
  "ex18.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex19-grid-failure.csv"
  "ex19-grid-failure.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex19-grid-repair.csv"
  "ex19-grid-repair.csv"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex19.toml"
  "ex19.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex20.toml"
  "ex20.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex21.toml"
  "ex21.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex22.toml"
  "ex22.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex23.toml"
  "ex23.toml"
  COPYONLY)
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/examples/ex24.toml"
  "ex24.toml"
  COPYONLY)
