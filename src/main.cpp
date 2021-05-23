#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <Debugger.h>

uintptr_t read_abs_load_addr(pid_t pid);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please specify a program to debug.\n";
        return EXIT_FAILURE;
    }

    auto prog = argv[1];    // program name

    pid_t pid = fork();
    if (pid != 0) {
        // Execute debugger (parent)
        Debugger debugger {prog, pid};
        debugger.run();
    } else {
        // Execute program to debug (child)
        Debugger::launch_process(prog, pid);
    }

}

