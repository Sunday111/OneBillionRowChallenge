cmake_minimum_required(VERSION 3.16)

set(target_name obrc)
set(target_sources_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE target_sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "${target_sources_dir}/*")
add_library(${target_name} STATIC ${target_sources})
