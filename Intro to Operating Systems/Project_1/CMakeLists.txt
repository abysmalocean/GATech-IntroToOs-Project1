cmake_minimum_required(VERSION 3.3)
project(Project_1)

set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES echoServer.c)
add_executable(Project_1 ${SOURCE_FILES})