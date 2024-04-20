cmake_minimum_required(VERSION 3.16)

set(project_name OneBillionRowChallenge)
project(${project_name} CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/deps/sunday_cmake_presets")
include(generic_compile_options)

set(target_name obrc)
set(target_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/code)
file(GLOB_RECURSE target_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${target_sources_dir}/*")
add_executable(${target_name} ${target_sources})
set_generic_compile_options(${target_name} PUBLIC)
