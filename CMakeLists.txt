cmake_minimum_required(VERSION 3.15)

# Change the compiler BEFORE the first `project()` command to avoid an infinite loop.
# See: https://public.kitware.com/pipermail/cmake/2009-November/033133.html
if(APPLE)
  # If custom llvm compilers are available, use them (for openmp
  # support), otherwise disable multicore.
  if(EXISTS "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
    set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
  endif()
endif()

# Sets the env vars PROJECT_SOURCE_DIR, and PROJECT_BINARY_DIR
project(zeth CXX)

# Versionning of the project
set(ZETH_VERSION_MAJOR 0)
set(ZETH_VERSION_MINOR 4)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Support for the 2011 ISO C++ standard was claimed from gcc4.8+:
  #     https://gcc.gnu.org/gcc-4.8/cxx0x_status.html
  # And some sanitizers we use (e.g. UBSan) were added in gcc4.9+:
  #     https://gcc.gnu.org/gcc-4.9/changes.html
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    message(FATAL_ERROR "We use c++11 and some sanitizers which require"
      " GCC version at least 4.9")
  endif ()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # The sanitizers we use were supported from clang 3.9
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.9)
    message(FATAL_ERROR "We use c++11 and some sanitizers which require"
      " Clang version at least 3.9")
  endif ()
endif ()

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  "${PROJECT_SOURCE_DIR}/zeth_config.h.in"
  "${PROJECT_BINARY_DIR}/zeth_config.h"
)

# Flags and compilation options to chose the type of zksnark
set(
  ZKSNARK
  "GROTH16"
  CACHE
  STRING
  "Default snark: one of PGHR13, GROTH16"
)

# Run only fast test (e.g. on CI machine)
option(
  FAST_TESTS_ONLY
  "Include only fast-running tests"
  OFF
)

# Flags and compilation options for use with libsnark
set(
  CURVE
  "ALT_BN128"
  CACHE
  STRING
  "Default curve: one of ALT_BN128, BN128, EDWARDS, MNT4, MNT6"
)

set(
  DEPENDS_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/depends"
)

set(
  OPT_FLAGS
  ""
  CACHE
  STRING
  "Override C++ compiler optimization flags"
)

# This option allows to set Zeth as a top-level project
# or a dependency. This option is used to manage conflicting
# targets definitions.
# See, discussion here for more details:
# https://github.com/clearmatics/zeth/pull/177#discussion_r399424467
option(
  IS_ZETH_PARENT
  "Zeth parent folder option"
  ON
)

option(
  MULTICORE
  "Enable parallelized execution, using OpenMP"
  ON
)

option(
  WITH_PROCPS
  "Use procps for memory profiling"
  ON
)

option(
  VERBOSE
  "Print internal messages"
  ON
)

option(
  DEBUG
  "Enable debugging mode"
  ON
)

option(
  BINARY_OUTPUT
  "Binary stream reading and writing"
  ON
)

option(
  MONTGOMERY_OUTPUT
  "Serialize Fp elements as their Montgomery representations (faster but not human-readable)"
  ON
)

option(
  USE_PT_COMPRESSION
  "Use point compression"
  OFF
)

option(
  GEN_DOC
  "Generate project documentation"
  OFF
)

option(
  CODE_COVERAGE
  "Generate code coverage report"
  OFF
)

if(APPLE)
  # These must be disabled to make dependencies build on macos
  set(WITH_PROCPS OFF)
  set(WITH_SUPERCOP OFF CACHE BOOL "Build libff with supercop")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

  # (Currently) OpenMP only available with custom llvm compilers
  if(${CMAKE_C_COMPILER} MATCHES ".*cc$")
    set(MULTICORE OFF)
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang$")
  # Common compilation flags and warning configuration
  # The CMAKE_CXX_FLAGS variable allows to change the compiler settings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors -pthread")

  if("${MULTICORE}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()

   # Default optimizations flags (to override, use -DOPT_FLAGS=...)
  if("${OPT_FLAGS}" STREQUAL "")
    set(OPT_FLAGS "-ggdb3 -O2 -march=native -mtune=native")
  endif()
else()
  message(WARNING "This project is assumed to be compiled with gcc or clang."
  " No guarantees can be made if another compiler is used.")
endif()

add_definitions(-DCURVE_${CURVE})
add_definitions(-DZKSNARK_${ZKSNARK})

enable_testing()

if(${CURVE} STREQUAL "BN128")
  add_definitions(-DBN_SUPPORT_SNARK=1)
endif()

if("${VERBOSE}")
  add_definitions(-DVERBOSE=1)
endif()

if("${MULTICORE}")
  add_definitions(-DMULTICORE=1)
endif()

if("${BINARY_OUTPUT}")
  add_definitions(-DBINARY_OUTPUT)
endif()

if("${MONTGOMERY_OUTPUT}")
  add_definitions(-DMONTGOMERY_OUTPUT)
endif()

if(NOT "${USE_PT_COMPRESSION}")
  add_definitions(-DNO_PT_COMPRESSION=1)
endif()

if("${DEBUG}")
  add_definitions(-DDEBUG=1)
endif()

# Add the given directories to those the compiler uses to search for include files
include_directories(.)

if ("${IS_ZETH_PARENT}")
  add_custom_target(
    check
    COMMAND
    ${CMAKE_CTEST_COMMAND}
  )
endif()

# Add dependencies (before setting up the configuration for zeth)
add_subdirectory(depends)

# CMAKE_MODULE_PATH: Path used for searching by FIND_XXX(), with appropriate
# suffixes added.
# See: https://cmake.org/cmake/help/v3.0/variable/CMAKE_PREFIX_PATH.html
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib" "/usr/lib")
# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Global gRPC and Protocol Buffers configuration.
# The libraries location can be known by running:
# `ldconfig -p | grep libgrpc`
# `ldconfig -p | grep libprotobuf`
#
# Set the target of the Protobuf and gRPC generated files.
set(
  PROTO_SRC_DIR
  ${PROJECT_BINARY_DIR}
)
#file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

# Get the proto files
file(
  GLOB
  PROTO_FILES
  ${PROJECT_SOURCE_DIR}/api/*.proto
)

# Generating sensibly into a python module requires setting --proto_path to the
# zeth root, and referencing files as 'api/prover.proto' etc. This requires us
# to tweak the references to the source files.
set(PROTOBUF_IMPORT_DIRS ${PROJECT_SOURCE_DIR})
set(PROTOBUF_PROTO_PATH ${PROJECT_SOURCE_DIR})
set(PROTOBUF_APPEND_DEST_PATH "/api")
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)
set(GRPC_GENERATE_CPP_APPEND_PATH OFF)

# By default, compile with all warning flags
add_compile_options(-Wall -Wextra)

# Add extra configuration files to run additional tooling on the code and to
# generate documentation and code coverage reports
#
# Import configuration to run dev tools on the code
# Generate the compile database needed for some of the tools we invoke below
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/gcc_dev_tools.cmake)
include(cmake/clang_dev_tools.cmake)

# Import configuration to compile targets with sanitizers
include(cmake/sanitizers.cmake)

# Import configuration to generate the code documentation if option is set
if("${GEN_DOC}")
    include(cmake/documentation.cmake)
endif()

# Import configuration to generate the coverage report if option is set
if("${CODE_COVERAGE}")
    include(cmake/code_coverage.cmake)
endif()

# Add all local subdirecetories
add_subdirectory(libzeth)
add_subdirectory(prover_server)
if(${ZKSNARK} STREQUAL "GROTH16")
  add_subdirectory(mpc_tools)
endif()
