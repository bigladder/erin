cmake_minimum_required(VERSION 3.10) # required for gtest_discover_tests

project(DISCO)
set(CMAKE_CXX_STANDARD 17)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option( ${CMAKE_PROJECT_NAME}_TESTING "Build DISCO testing targets" OFF)
option( ${CMAKE_PROJECT_NAME}_COVERAGE "Generate coverage reports" OFF)

# Set up testing/coverage
if (${CMAKE_PROJECT_NAME}_TESTING)
  enable_testing()
  if (${CMAKE_PROJECT_NAME}_COVERAGE)
    set(ENABLE_COVERAGE ON CACHE BOOL "" FORCE)
    find_package(codecov)
  endif()
endif()

# Add subdirectories
add_subdirectory(vendor)
add_subdirectory(src)

if (${CMAKE_PROJECT_NAME}_TESTING)
  add_subdirectory(test)
  if (${CMAKE_PROJECT_NAME}_COVERAGE)
    coverage_evaluate()
  endif()
endif()

option(BUILD_DOC "Build documentation" ON)

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
