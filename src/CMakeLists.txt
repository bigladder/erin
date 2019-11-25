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
set(sources
  ${PROJECT_SOURCE_DIR}/src/erin.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_component.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_element.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_fragility.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_network.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_stream.cpp
  ${PROJECT_SOURCE_DIR}/src/erin_type.cpp
)

set(public_headers
  ${PROJECT_SOURCE_DIR}/include/erin/component.h
  ${PROJECT_SOURCE_DIR}/include/erin/distribution.h
  ${PROJECT_SOURCE_DIR}/include/erin/element.h
  ${PROJECT_SOURCE_DIR}/include/erin/erin.h
  ${PROJECT_SOURCE_DIR}/include/erin/fragility.h
  ${PROJECT_SOURCE_DIR}/include/erin/network.h
  ${PROJECT_SOURCE_DIR}/include/erin/port.h
  ${PROJECT_SOURCE_DIR}/include/erin/stream.h
  ${PROJECT_SOURCE_DIR}/include/erin/type.h
)

set(private_headers
  ${PROJECT_SOURCE_DIR}/src/debug_utils.h
  ${PROJECT_SOURCE_DIR}/src/erin_csv.h
  ${PROJECT_SOURCE_DIR}/src/erin_generics.h
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
else()
  set(CMAKE_MACOSX_RPATH 1)
  add_library(${CMAKE_PROJECT_NAME} SHARED ${library_sources})
endif()

include(GenerateExportHeader)
generate_export_header(${CMAKE_PROJECT_NAME})

# target_link_libraries(${CMAKE_PROJECT_NAME} vendor dependencies)

if (${CMAKE_PROJECT_NAME}_COVERAGE)
  add_coverage(${CMAKE_PROJECT_NAME})
endif()

add_executable(e2rin cli.cpp ${sources} ${public_headers} ${private_headers})
add_executable(str8760 stress_test_8760.cpp ${sources} ${public_headers} ${private_headers})
add_executable(str8760x5000 stress_test_8760x5000.cpp ${sources} ${public_headers} ${private_headers})
add_executable(limits limits.cpp)

# Add Warnings
if(MSVC)
  target_compile_options(e2rin PRIVATE /W4 "$<$<CONFIG:RELEASE>:/O2>")
  target_compile_options(str8760 PRIVATE /W4 "$<$<CONFIG:RELEASE>:/O2>")
  target_compile_options(str8760x5000 PRIVATE /W4 "$<$<CONFIG:RELEASE>:/O2>")
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4 "$<$<CONFIG:RELEASE>:/O2>")
else()
  target_compile_options(e2rin PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:RELEASE>:-O3>")
  target_compile_options(str8760 PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:RELEASE>:-O3>")
  target_compile_options(str8760x5000 PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:RELEASE>:-O3>")
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic "$<$<CONFIG:RELEASE>:-O3>")
endif()
