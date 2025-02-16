cmake_minimum_required(VERSION 2.8...3.15)
project(eargs LANGUAGES C CXX)

get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)
if(NOT IS_SUBPROJECT)
    add_subdirectory(tests)
endif()

add_library(${PROJECT_NAME} INTERFACE) 
target_include_directories(${PROJECT_NAME} INTERFACE includes)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)