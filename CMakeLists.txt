# band3 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(band3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
file(GLOB_RECURSE BAND3_SOURCES src/*.cpp src/*.c)

if(WIN32)
    add_executable(band3 WIN32 ${BAND3_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/band3.rc)
else()
    add_executable(band3 ${BAND3_SOURCES})
endif()

rexglue_setup_target(band3 GPU_PLUGINS xenos)
