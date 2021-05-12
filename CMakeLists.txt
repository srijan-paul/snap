cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(APP_VERSION "0.0.1" CACHE STRING "Application Version" FORCE)
project(vyse VERSION ${APP_VERSION}
             DESCRIPTION "An embeddable, dynamically typed language."
             LANGUAGES CXX)
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lang/src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
set(INCLUDE_DIR lang/include/vyse)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

enable_testing()

function(LINK_VYSE_DEPS EXEC)
    target_link_libraries(${EXEC} ${PROJECT_NAME})
    target_include_directories(${EXEC} PUBLIC ${INCLUDE_DIR})
endfunction()

# static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall" "-Wpedantic" "-Wextra" "-fexceptions")

# cli app
set(CLI_NAME "vy")
add_executable(${CLI_NAME} cli/main.cpp)
target_link_libraries(${CLI_NAME} ${PROJECT_NAME})
target_compile_features(${CLI_NAME} PRIVATE cxx_std_17)

# tests
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(TEST_UTIL_SRC "${TEST_DIR}/util/test_utils.cpp")
function(PREPARE_TEST EXE_NAME TEST_NAME FILE_NAME)
    add_executable(${EXE_NAME} "${TEST_DIR}/${FILE_NAME}" ${TEST_UTIL_SRC})
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    LINK_VYSE_DEPS(${EXE_NAME})
    add_test(${TEST_NAME} ${EXE_NAME})
endfunction()

PREPARE_TEST(scanner_test ScannerTests "scanner_test.cpp")
PREPARE_TEST(compiler_test CompilerTests "compiler_test.cpp")
PREPARE_TEST(vm_test VMTest "vm_test.cpp")
PREPARE_TEST(table_test TableTest "table_test.cpp")
PREPARE_TEST(gc-test GCTest "gc-test.cpp")
PREPARE_TEST(stdlib-test StdlibTest "stdlib-test.cpp")
PREPARE_TEST(auto-tests AutoTests "auto-tests.cpp")

# benchmarks
set(BENCHMARK_SUITE "benchmark")
add_executable(${BENCHMARK_SUITE} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/cpp/benchmark.cpp)
LINK_VYSE_DEPS(${BENCHMARK_SUITE})
target_compile_features(${BENCHMARK_SUITE} PUBLIC cxx_std_17)