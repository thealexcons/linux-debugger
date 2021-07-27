# Simple Linux Debugger

A simple command-line debugger for Linux-based ELF binaries, inspired by GDB. This is my first ever C++ project; I used it to familiarise myself with STL containers and algorithms, and also to interface with lower-level system APIs such as ``ptrace`` and other process management syscalls.


## Getting Started

### Requirements:

- ``g++`` or ``gcc`` compiler
- CMake 

### Installation:

1. Clone this repository and ``cd`` into it the cloned directory.
2. Run ``cmake`` to build the binary.


## Using the Debugger

To debug a program, run ``./LinuxDebugger <path-to-your-program>``.

**Important:** For an enhanced debugging experience, make sure to compile your programs with the ``-g`` flag.

### Features and Commands:

- **Setting breakpoints:** todo
- **Stepping:**
  - fdsfs
  - fsd
- **Continue:** todo
- **Print registers:** todo
- **Print memory:** todo
