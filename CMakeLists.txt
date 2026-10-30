cmake_minimum_required(VERSION 3.20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(evaluation_algorithms_trends
    VERSION 1.0.0
    DESCRIPTION "Tokamak System Monitor Evaluation algorithms for trend analysis"
    LANGUAGES CXX
)
set(TSM_DISPLAY_NAME "TSM Evaluation algorithms for trend analysis")

enable_testing()

# ---------------------------------------------------------------------------
# Global C++ settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer position-independent code for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Standard output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ---------------------------------------------------------------------------
# Extra compilation directories
# ---------------------------------------------------------------------------
set(EXTRA_INC_DIRS "" CACHE STRING "Extra include directories")
set(EXTRA_LIB_DIRS "" CACHE STRING "Extra library directories")
if(NOT EXTRA_INC_DIRS AND DEFINED ENV{CODAC_ROOT})
    set(EXTRA_INC_DIRS "$ENV{CODAC_ROOT}/include" CACHE STRING "Extra include directories" FORCE)
endif()
if(NOT EXTRA_LIB_DIRS AND DEFINED ENV{CODAC_ROOT})
    set(EXTRA_LIB_DIRS "$ENV{CODAC_ROOT}/lib" CACHE STRING "Extra library directories" FORCE)
endif()

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(BUILD_TESTS            "Build tests"                                 ON)
option(BUILD_DOCUMENTATION    "Build documentation"                         OFF)

# --------------------------
# Find system packages
# --------------------------
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Build option
# ---------------------------------------------------------------------------
if(TSM_DEBUG)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  string(ASCII 27 Esc)
  message(STATUS "${Esc}[103;1;30m>>> DEBUG MODE ACTIVATED: Compiling with debugging symbols! <<<${Esc}[m")
endif()

# ---------------------------------------------------------------------------
# Build algorithm
# ---------------------------------------------------------------------------
add_subdirectory(src/c++)
# TODO: COPY headers in include folder (Check Victor's CMakeLists.txt)

# ---------------------------------------------------------------------------
# Build tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ------ -------------------------- ---- ------------------ ------ ----------- ----
# Coverage generation target
# ------ -------------------------- ---- ------------------ ------ ----------- ----

set(COVERAGE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src/c++/*")
set(COVERAGE_OMIT "/tmp/*,${CMAKE_CURRENT_SOURCE_DIR}/tests/*")

configure_file(cmake/coverage.sh.in "${CMAKE_BINARY_DIR}/cmake/coverage.sh" @ONLY)

add_custom_target(coverage
    COMMAND bash "${CMAKE_BINARY_DIR}/cmake/coverage.sh"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating code coverage report..."
    VERBATIM
)
message(STATUS "Coverage target configured")


# ---------------------------------------------------------------------------
# Build documentation
# ---------------------------------------------------------------------------
if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()
