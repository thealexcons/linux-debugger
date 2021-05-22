cmake_minimum_required(VERSION 3.19)
project(LinuxDebugger)

set(CMAKE_CXX_STANDARD 17)

include_directories(include ext/libelfin ext/linenoise)
add_executable(LinuxDebugger src/main.cpp src/Debugger.cpp ext/linenoise/linenoise.c include/Utils.h)