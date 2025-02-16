cmake_minimum_required(VERSION 2.8...3.15)
project(eargs-tests LANGUAGES C CXX)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(${PROJECT_NAME} source/hex.cpp source/string.cpp)
target_link_libraries(${PROJECT_NAME} GTest::gtest_main eargs)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)