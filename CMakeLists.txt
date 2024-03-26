# Copyright (c) 2020 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.
cmake_minimum_required(VERSION 3.15..3.28) # required for gtest_discover_tests

project(ERIN
    VERSION 0.0.55
    DESCRIPTION "Energy Resilience of Interacting Networks"
    LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
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
  # add_subdirectory(test)
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

