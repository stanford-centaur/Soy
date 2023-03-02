cmake_minimum_required (VERSION 3.12)
project(Soy)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_STATIC_SOY "Build static Soy binary" OFF)
option(RUN_UNIT_TEST "run unit tests on build" ON)
option(RUN_MEMORY_TEST "run cxxtest testing with ASAN ON" ON)
option(CODE_COVERAGE "add code coverage" OFF)  # Available only in debug mode

set(SOY_LIB SoyHelper)
set(SOY_TEST_LIB SoyHelperTest)
set(SOY_EXE Soy${CMAKE_EXECUTABLE_SUFFIX})

set(DEPS_DIR "${PROJECT_SOURCE_DIR}/deps")
set(TOOLS_DIR "${PROJECT_SOURCE_DIR}/tools")
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(ENGINE_DIR "${SRC_DIR}/engine")
set(PYBIND11_DIR "${TOOLS_DIR}/pybind11-2.3.0")
set(BOOST_DIR "${TOOLS_DIR}/boost_1_68_0")
set(CADICAL_DIR "${TOOLS_DIR}/cadical")

set(COMMON_DIR "${SRC_DIR}/common")
set(CVC4_DIR "${DEPS_DIR}/CVC4")

set(LOCAL_SEARCH_DIR "${SRC_DIR}/local_search")
set(CONSTRAINTS_DIR "${SRC_DIR}/constraints")

set(CONTEXT_DIR "${CVC4_DIR}/context")
set(CVC4_BASE_DIR "${CVC4_DIR}/base")
file(GLOB DEPS_CVC4_CONTEXT "${CONTEXT_DIR}/*.cpp")
file(GLOB DEPS_CVC4_BASE "${CVC4_BASE_DIR}/*.cpp")
include_directories(SYSTEM ${CVC4_DIR} ${CVC4_DIR}/include)

set(CADICAL_LIB cadical)

set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")

set(COMMON_REAL "${COMMON_DIR}/real")
set(COMMON_MOCK "${COMMON_DIR}/mock")
file(GLOB SRCS_COMMON_REAL "${COMMON_REAL}/*.cpp")
file(GLOB SRCS_COMMON_MOCK "${COMMON_MOCK}/*.cpp")

set(ENGINE_REAL "${ENGINE_DIR}/real")
set(ENGINE_MOCK "${ENGINE_DIR}/mock")
file(GLOB SRCS_ENGINE_REAL "${ENGINE_REAL}/*.cpp")
file(GLOB SRCS_ENGINE_MOCK "${ENGINE_MOCK}/*.cpp")

set(MPS_PARSER mps)
set(ACAS_PARSER acas)
set(BERKELEY_PARSER berkeley)
set(INPUT_PARSERS_DIR input_parsers)

#-----------------------------------------------------------------------------#
# Determine number of threads available, used to configure (default) parallel
# execution of custom test targets (can be overriden with ARGS=-jN).

include(ProcessorCount)
ProcessorCount(CTEST_NTHREADS)
if(CTEST_NTHREADS EQUAL 0)
  set(CTEST_NTHREADS 1)
endif()
 
# --------------- set build type ----------------------------
set(BUILD_TYPES Release Debug MinSizeRel RelWithDebInfo)

# Set the default build type to Production
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release CACHE STRING "Options are: Release Debug MinSizeRel RelWithDebInfo" FORCE)
  # Provide drop down menu options in cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
endif()
message(STATUS "Building ${CMAKE_BUILD_TYPE} build")

#-------------------------set code coverage----------------------------------#
# Allow coverage only in debug mode only in gcc
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Building with code coverage")
  set(COVERAGE_COMPILER_FLAGS "-g -O0 --coverage" CACHE INTERNAL "")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# We build a static library that is the core of the project, the link it to the
# API's (executable and python at the moment)
add_library(${SOY_LIB} ${DEPS_CVC4_CONTEXT} ${DEPS_CVC4_BASE} ${SRCS_COMMON_REAL} ${SRCS_ENGINE_REAL})
target_include_directories(${SOY_LIB} PRIVATE SYSTEM)

add_executable(${SOY_EXE} "${ENGINE_DIR}/main.cpp")
set(SOY_EXE_PATH "${BIN_DIR}/${SOY_EXE}")
add_custom_command(TARGET ${SOY_EXE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${SOY_EXE}> ${SOY_EXE_PATH} )

set(MPS_PARSER_PATH "${BIN_DIR}/${MPS_PARSER}")

if (NOT MSVC)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
       set(COMPILE_FLAGS  -Wall -Wextra -MMD -Qunused-arguments ) #-Wno-deprecated
     else()
       set(COMPILE_FLAGS  -Wall -Wextra -MMD ) #-Wno-deprecated
    endif()
    set(RELEASE_FLAGS ${COMPILE_FLAGS} -O3) #-Wno-deprecated
endif()

if (RUN_MEMORY_TEST)
    if(NOT MSVC)
        set(MEMORY_FLAGS -fsanitize=address -fno-omit-frame-pointer -O1)
    endif()
endif()

if (NOT MSVC)
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS} -g)
    set(CXXTEST_FLAGS ${DEBUG_FLAGS}  -Wno-ignored-qualifiers)
else()
    set(DEBUG_FLAGS ${COMPILE_FLAGS} ${MEMORY_FLAGS})
    add_definitions(-DNOMINMAX) # remove min max macros
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CXXTEST_FLAGS ${CXXTEST_FLAGS} -Wno-terminate)
endif()

# Boost
if (MSVC)
    set(BOOST_ROOT "${BOOST_DIR}/win_installed")
elseif (${CMAKE_SIZEOF_VOID_P} EQUAL 4 AND NOT MSVC)
    set(BOOST_ROOT "${BOOST_DIR}/installed32")
else()
    set(BOOST_ROOT "${BOOST_DIR}/installed")
endif()

if (MSVC)
    set(SCRIPT_EXTENSION bat)
    set(Boost_NAMESPACE libboost)
else()
    set(SCRIPT_EXTENSION sh)
endif()

if (MSVC)
    set(SCRIPT_EXTENSION bat)
    set(Boost_NAMESPACE libboost)
else()
    set(SCRIPT_EXTENSION sh)
endif()
find_package(Boost COMPONENTS program_options timer chrono thread)
# Find boost
if (NOT ${Boost_FOUND})
    # bash file that downloads and install boost 1_68_0, the name need to match
    # BOOST_DIR variable
    execute_process(COMMAND ${TOOLS_DIR}/download_boost.${SCRIPT_EXTENSION})
    find_package(Boost REQUIRED COMPONENTS program_options timer chrono thread)
endif()
set(LIBS_INCLUDES ${Boost_INCLUDE_DIRS})
set(LIBS ${Boost_LIBRARIES})

message(STATUS "Looking for Gurobi")
if (NOT DEFINED GUROBI_DIR)
  set(GUROBI_DIR $ENV{GUROBI_HOME})
endif()
add_compile_definitions(ENABLE_GUROBI)

set(GUROBI_LIB1 "gurobi_c++")
set(GUROBI_LIB2 "gurobi95")

add_library(${GUROBI_LIB1} SHARED IMPORTED)
set_target_properties(${GUROBI_LIB1} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi_c++.a)
list(APPEND LIBS ${GUROBI_LIB1})
target_include_directories(${GUROBI_LIB1} INTERFACE ${GUROBI_DIR}/include/)

add_library(${GUROBI_LIB2} SHARED IMPORTED)

# MACOSx uses .dylib instead of .so for its Gurobi downloads.
if (APPLE)
  set_target_properties(${GUROBI_LIB2} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi95.dylib)
else()
  set_target_properties(${GUROBI_LIB2} PROPERTIES IMPORTED_LOCATION ${GUROBI_DIR}/lib/libgurobi95.so)
endif ()

list(APPEND LIBS ${GUROBI_LIB2})
target_include_directories(${GUROBI_LIB2} INTERFACE ${GUROBI_DIR}/include/)

# Cadical
message(STATUS "Looking for cadical")
if(NOT EXISTS "${CADICAL_DIR}/build/libcadical.a")
  message("Can't find cadical, installing.")
  execute_process(COMMAND ${TOOLS_DIR}/download_cadical.sh)
endif()

add_library(${CADICAL_LIB} SHARED IMPORTED)
set_target_properties(${CADICAL_LIB} PROPERTIES IMPORTED_LOCATION ${CADICAL_DIR}/build/libcadical.a)
list(APPEND LIBS ${CADICAL_LIB})
target_include_directories(${CADICAL_LIB} INTERFACE ${CADICAL_DIR}/src/)

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND LIBS Threads::Threads)

if (BUILD_STATIC_SOY)
  # build a static library
  target_link_libraries(${SOY_LIB} ${LIBS} -static)
else()
  target_link_libraries(${SOY_LIB} ${LIBS})
endif()

target_include_directories(${SOY_LIB} PRIVATE ${LIBS_INCLUDES})
target_compile_options(${SOY_LIB} PRIVATE ${RELEASE_FLAGS})

# Build Soy executable
target_link_libraries(${SOY_EXE} ${SOY_LIB})
target_include_directories(${SOY_EXE} PRIVATE ${LIBS_INCLUDES})

add_library(${SOY_TEST_LIB})
set (TEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests")
file(MAKE_DIRECTORY ${TEST_DIR})

set(CMAKE_PREFIX_PATH "${TOOLS_DIR}/cxxtest")
set(CXXTEST_USE_PYTHON FALSE)
find_package(CxxTest)
if(CXXTEST_FOUND)
    include_directories(${CXXTEST_INCLUDE_DIR})
    enable_testing()
endif()

target_link_libraries(${SOY_TEST_LIB} ${SOY_LIB} ${LIBS})
target_include_directories(${SOY_TEST_LIB} PRIVATE ${LIBS_INCLUDES} )
target_compile_options(${SOY_TEST_LIB} PRIVATE ${CXXTEST_FLAGS})

add_custom_target(build-tests ALL)

add_custom_target(check
      COMMAND ctest --output-on-failure -j${CTEST_NTHREADS} $$ARGS
      DEPENDS build-tests build_input_parsers ${SOY_EXE})

# Decide which tests to run and execute
set(TESTS_TO_RUN "")
# ctest uses regex, so create the string to look: (unit|system) ...
macro(append_tests_to_run new_val)
    if ("${TESTS_TO_RUN}" STREQUAL "")
        set(TESTS_TO_RUN ${new_val})
    else()
        set(TESTS_TO_RUN "${TESTS_TO_RUN}|${new_val}")
    endif()
endmacro()

if (${RUN_UNIT_TEST})
    append_tests_to_run("unit")
endif()
if (${RUN_SYSTEM_TEST})
    append_tests_to_run("system")
endif()
if (NOT ${TESTS_TO_RUN} STREQUAL "")
    # make ctest verbose
    set(CTEST_OUTPUT_ON_FAILURE 1)
    add_custom_command(
        TARGET build-tests
        POST_BUILD
        COMMAND ctest --output-on-failure  -L "\"(${TESTS_TO_RUN})\"" -j${CTEST_NTHREADS} $$ARGS
    )
endif()

# Add the input parsers
add_custom_target(build_input_parsers)
add_dependencies(build_input_parsers ${MPS_PARSER})

add_subdirectory(${SRC_DIR})
add_subdirectory(${TOOLS_DIR})


execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Soy Version
set(SOY_VERSION 1.0.+)
add_definitions("-DGIT_COMMIT_HASH=\"${GIT_COMMIT_HASH}\"")
add_definitions("-DGIT_BRANCH=\"${GIT_BRANCH}\"")
add_definitions("-DSOY_VERSION=\"${SOY_VERSION}\"")
