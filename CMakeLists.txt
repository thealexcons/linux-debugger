cmake_minimum_required(VERSION 3.19)
project(LinuxDebugger)

set(CMAKE_CXX_STANDARD 17)

include_directories(include ext/libelfin ext/linenoise)
add_executable(LinuxDebugger ext/linenoise/linenoise.c src/main.cpp src/Debugger.cpp src/Breakpoint.cpp src/DwarfContext.cpp)

# Setup libelfin library
add_custom_target(
        libelfin
        COMMAND make
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/ext/libelfin
)
target_link_libraries(LinuxDebugger
        ${PROJECT_SOURCE_DIR}/ext/libelfin/dwarf/libdwarf++.so
        ${PROJECT_SOURCE_DIR}/ext/libelfin/elf/libelf++.so)
add_dependencies(LinuxDebugger libelfin)