# Simple Linux Debugger

A simple command-line debugger for Linux-based ELF binaries, inspired by GDB. 


## Getting Started

### Requirements:

- ``g++`` or ``gcc`` compiler
- ``cmake`` build tool 

### Installation:

1. Clone this repository and ``cd`` into it the cloned directory.
2. Run ``cmake . && make`` to build the binary.
3. This will generate a ``LinuxDebugger`` binary file in the current directory.


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
