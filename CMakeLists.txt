cmake_minimum_required(3.14 FATAL_ERROR)

project(KaleidoscopeCompiler VERSION 0.0
        DESCRIPTION "A simple Kaleidoscope language compiler"
        LANGUAGES CXX)

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

include_directories("${CMAKE_SOURCE_DIR}/include")
