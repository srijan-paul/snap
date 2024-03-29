cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(APP_VERSION "0.0.1" CACHE STRING "Application Version" FORCE)
project(vyse VERSION ${APP_VERSION}
             DESCRIPTION "An embeddable, dynamically typed language."
             LANGUAGES CXX)
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
set(INCLUDE_DIR include/)

set(STRESS_GC OFF CACHE BOOL "Run garbage collector as often as possible")
set(LOG_GC OFF CACHE BOOL "Log GC output on every GC event.")
set(LOG_DISASM OFF CACHE BOOL "Log program disassembly before execution.")
set(BUILD_TESTS ON CACHE BOOL "Compile the test suite.")
set(VYSE_MINSTACK OFF CACHE STRING "When the VM stack is first initialized, have it be as small as possible.")

if (UNIX AND NOT APPLE)
	set(LINUX true)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(BUILD_TESTS)
  enable_testing()
endif()

function(LINK_VYSE_DEPS EXEC)
    target_link_libraries(${EXEC} ${PROJECT_NAME})
    target_include_directories(${EXEC} PUBLIC ${INCLUDE_DIR})
endfunction()

# The dino helper is used for dynamic library loading.
add_subdirectory("ext/dino")

# The vyse static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC "ext/dino/include")
target_link_libraries(${PROJECT_NAME} PRIVATE dino::dino)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Configure some macros based on flags.
if(STRESS_GC)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DVYSE_STRESS_GC)
endif()

if(LOG_GC)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DVYSE_LOG_GC)
endif()

if(VYSE_MINSTACK)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DVYSE_MINSTACK=4)
endif()

if(LOG_DISASM)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DVYSE_DEBUG_DISASSEMBLY)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC  $<$<CONFIG:Debug>:VYSE_DEBUG>)

# exclude windows' stupid lowercase macros like 'min' and 'max'
# to stop them from interfering with vyse code.
target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall" "-Wpedantic" "-Wextra" "-fexceptions")

# Builds a shared object file from a vyse standard library source file
function(BUILD_VYSE_LIB LIB_NAME)
  add_library(${LIB_NAME} SHARED ${SOURCE_DIR}/stdlib/${LIB_NAME}.cpp)
  target_link_libraries(${LIB_NAME} PRIVATE ${PROJECT_NAME})
  target_include_directories(${LIB_NAME} PRIVATE ${INCLUDE_DIR})
  target_compile_features(${LIB_NAME} PRIVATE cxx_std_17)
endfunction(BUILD_VYSE_LIB)

# shared libraries for the vyse stdlib
BUILD_VYSE_LIB(vymath)

# cli app
set(CLI_NAME "vy")
add_executable(${CLI_NAME} cli/main.cpp)
target_link_libraries(${CLI_NAME} ${PROJECT_NAME})
target_compile_features(${CLI_NAME} PRIVATE cxx_std_17)

# tests
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(TEST_UTIL_SRC "${TEST_DIR}/util/test_utils.cpp")
function(PREPARE_TEST EXE_NAME TEST_NAME FILE_NAME)
    add_executable(${EXE_NAME} "${TEST_DIR}/${FILE_NAME}" ${TEST_UTIL_SRC})
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
		if(LINUX)
        target_link_libraries(${EXE_NAME} stdc++fs)
		endif()
    LINK_VYSE_DEPS(${EXE_NAME})
    add_test(${TEST_NAME} ${EXE_NAME})
endfunction()

if(BUILD_TESTS)
  PREPARE_TEST(scanner-test ScannerTests "scanner-test.cpp")
  PREPARE_TEST(compiler-test CompilerTests "compiler-test.cpp")
  PREPARE_TEST(vm-test VMTest "vm-test.cpp")
  PREPARE_TEST(table-test TableTest "table-test.cpp")
  PREPARE_TEST(gc-test GCTest "gc-test.cpp")
  PREPARE_TEST(stdlib-test StdlibTest "stdlib-test.cpp")
  PREPARE_TEST(auto-test AutoTests "auto-tests.cpp")
  PREPARE_TEST(udata-test AutoTests "udata-test.cpp")
endif()
