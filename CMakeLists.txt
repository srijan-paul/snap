cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(APP_VERSION "0.0.1" CACHE STRING "Application Version" FORCE)
project(snap VERSION ${APP_VERSION})
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lang/src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
set(INCLUDE_DIR lang/include/snap)
add_compile_options("-Wall" "-Wpedantic" "-Wextra" "-fexceptions")
enable_testing()

function(LINK_SNAP_DEPS EXEC)
    target_link_libraries(${EXEC} ${PROJECT_NAME})
    target_include_directories(${EXEC} PUBLIC ${INCLUDE_DIR})
endfunction()


# static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# cli app
set(CLI_NAME "snp")
add_executable(${CLI_NAME} src/main.cpp)
LINK_SNAP_DEPS(${CLI_NAME})
target_compile_features(${CLI_NAME} PRIVATE cxx_std_17)

# tests

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(TEST_UTIL_SRC "${TEST_DIR}/test_utils.cpp")
function(PREPARE_TEST EXE_NAME TEST_NAME FILE_NAME)
    add_executable(${EXE_NAME} "${TEST_DIR}/${FILE_NAME}" ${TEST_UTIL_SRC})
    target_compile_features(${EXE_NAME} PRIVATE cxx_std_17)
    LINK_SNAP_DEPS(${EXE_NAME})
    add_test(${TEST_NAME} ${EXE_NAME})
endfunction()

PREPARE_TEST(scanner_test ScannerTests "scanner_test.cpp")
PREPARE_TEST(compiler_test CompilerTests "compiler_test.cpp")
PREPARE_TEST(vm_test VMTest "vm_test.cpp")

# benchmarks
set(BENCHMARK_SUITE "benchmark")
add_executable(${BENCHMARK_SUITE} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/cpp/benchmark.cpp)
LINK_SNAP_DEPS(${BENCHMARK_SUITE})
target_compile_features(${BENCHMARK_SUITE} PRIVATE cxx_std_17)