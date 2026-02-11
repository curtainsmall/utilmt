cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

project(utilmt)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*/*.hpp")
add_library(utilmt INTERFACE ${sources})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${sources})
target_include_directories(utilmt INTERFACE "${CMAKE_SOURCE_DIR}/include")
target_compile_features(utilmt INTERFACE cxx_std_20)
set_target_properties(utilmt PROPERTIES FOLDER utilmt)

option(UTILMT_BUILD_TESTS "Build Tests" OFF)
if(UTILMT_BUILD_TESTS)
    add_subdirectory(tests)
endif()