//
// Created by alexcons on 22/05/2021.
//
#include <sys/wait.h>
#include <linenoise.h>
#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>
#include "Debugger.h"
#include "Utils.h"

// Launch the process to be debugged (pid must be 0, ie: called by child)
void Debugger::launch_process(const char *prog_name, pid_t pid) {
    ptrace(PTRACE_TRACEME, pid, nullptr, nullptr); // Trace the child process
    execl(prog_name, prog_name, nullptr);
}


// Run the debugger
void Debugger::run() {
    // Wait until SIGTRAP signal is sent to the child (at launch or by software interrupt)
    int wait_status;
    waitpid(_pid, &wait_status, 0);

    char *cmd;
    while ((cmd = linenoise("> ")) != nullptr) {
        handle(cmd);
        linenoiseHistoryAdd(cmd);
        linenoiseFree(cmd);
    }
}

// Handle user commands
void Debugger::handle(const std::string& line) {
    auto args = split_by(line, ' ');
    auto cmd = args[0];

    if (is_prefixed_by(cmd, "continue")) {
        continue_cmd();
    } else {
        std::cerr << "Unknown command\n";
    }
}


void Debugger::continue_cmd() {
    // Tell the process to continue and wait until another signal is sent
    ptrace(PTRACE_CONT, _pid, nullptr, nullptr);

    int wait_status;
    waitpid(_pid, &wait_status, 0);
}
