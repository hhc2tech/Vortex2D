cmake_minimum_required(VERSION 3.2)

project(Vortex2D)

set(CMAKE_CXX_STANDARD 14)

# setup version numbers
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

option(VORTEX2D_ENABLE_EXAMPLES "Build examples" OFF)
option(VORTEX2D_ENABLE_TESTS "Build tests" OFF)
option(VORTEX2D_ENABLE_DOCS "Build docs" OFF)

# Only do coverage builds for gcc for the moment
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_COVERAGE
        "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the C++ compiler during coverage builds."
        FORCE )
    set(CMAKE_C_FLAGS_COVERAGE
        "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the C compiler during coverage builds."
        FORCE )
endif ()

# cmake file to download and include dependencies
include(cmake/Macros.cmake)
include(cmake/DownloadProject.cmake)
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")

add_subdirectory(Vortex2D)

if (VORTEX2D_ENABLE_EXAMPLES)
  add_subdirectory(Examples)
endif ()

if (VORTEX2D_ENABLE_TESTS)
  add_subdirectory(Tests)
endif ()

if (VORTEX2D_ENABLE_DOCS)
  add_subdirectory(Docs)
endif ()
