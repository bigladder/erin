include(initialize-submodules)
initialize_submodules()

if (NOT TARGET fmt)
  add_subdirectory(fmt)
  set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
  mark_as_advanced(FMT_CMAKE_DIR FMT_CUDA_TEST FMT_DEBUG_POSTFIX FMT_DOC FMT_FUZZ FMT_INC_DIR FMT_INSTALL FMT_INSTALL
              FMT_LIB_DIR FMT_MODULE FMT_OS FMT_PEDANTIC FMT_PKGCONFIG_DIR FMT_SYSTEM_HEADERS FMT_TEST FMT_WERROR)
endif ()

# googletest library
if (${CMAKE_PROJECT_NAME}_TESTING AND NOT TARGET gtest)

  # Prevent GoogleTest from overriding our compiler/linker options
  # when building with Visual Studio
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GTEST ON CACHE BOOL "" FORCE MARK)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  mark_as_advanced(BUILD_GTEST BUILD_GMOCK INSTALL_GTEST)

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/googletest)

endif()

# CLI11 library
if (NOT TARGET CLI11)
  add_subdirectory(CLI11)
endif()

if (NOT TARGET courier)
  add_subdirectory(courier)
endif()
