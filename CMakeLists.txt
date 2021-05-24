cmake_minimum_required(VERSION 3.19)
project(LinuxDebugger)

set(CMAKE_CXX_STANDARD 20)


include_directories(include ext/libelfin ext/linenoise)
add_executable(LinuxDebugger ext/linenoise/linenoise.c src/main.cpp src/Debugger.cpp src/Breakpoint.cpp)
