cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
set(APP_VERSION "0.0.1" CACHE STRING "Application Version" FORCE)
project(snap VERSION ${APP_VERSION})
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lang/src")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
set(INCLUDE_DIR lang/include/snap)

enable_testing()

# static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# cli app
set(CLI_NAME "snp")
add_executable(${CLI_NAME} src/main.cpp)
target_link_libraries(${CLI_NAME} ${PROJECT_NAME})
target_include_directories(${CLI_NAME} PUBLIC ${INCLUDE_DIR})
target_compile_features(${CLI_NAME} PRIVATE cxx_std_17)

# tests
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")
add_executable(tests "${TEST_DIR}/test.cpp")
target_link_libraries(tests ${PROJECT_NAME})
target_include_directories(tests PUBLIC ${INCLUDE_DIR})
target_compile_features(tests PRIVATE cxx_std_17)
