cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(KaleidoscopeCompiler VERSION 0.0
        DESCRIPTION "A simple Kaleidoscope language compiler"
        LANGUAGES CXX)

include("${CMAKE_SOURCE_DIR}/cmake/utils/GetLLVM.cmake")

#- options -#
option(BUILD_TEST "whether build test for this project or not" ON)

#- settings -#
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

if(WIN32) # windows does not have rpath
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

# TODO: see https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html#guide:Using%20Dependencies%20Guide

#- modules managed by cmake -#
include("${CMAKE_SOURCE_DIR}/cmake/module/LLVM.cmake")

include_directories("${CMAKE_SOURCE_DIR}/include"
                    "${CMAKE_SOURCE_DIR}/3rdparty/nameof/include")

add_subdirectory("${CMAKE_SOURCE_DIR}/src/lexer")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/parser")
