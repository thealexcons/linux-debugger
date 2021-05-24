//
// Created by alexcons on 22/05/2021.
//

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <unordered_map>
#include "Breakpoint.h"

class Debugger {
public:
    Debugger (const std::string& prog_name, pid_t pid) {
        _prog_name = prog_name;
        _pid = pid;
        _abs_load_addr = UINTPTR_MAX;
    }

    static void launch_process(const char *prog_name, pid_t pid);

    // Debugger API
    void run();
    void set_breakpoint(std::uintptr_t addr);
    void remove_breakpoint(uintptr_t addr);
    void disable_breakpoint(uintptr_t addr);
    void continue_execution();

    void print_registers() const;

private:
    std::string _prog_name;
    pid_t _pid;
    std::uintptr_t _abs_load_addr;
    std::unordered_map<std::uintptr_t, Breakpoint> _breakpoints;

    // Read/write memory (via ptrace)
    uint64_t read_memory(uint64_t addr) const;
    uint64_t write_memory(uint64_t addr, uint8_t val) const;

    // Step over breakpoints
    uint64_t get_pc() const;
    void set_pc(uint64_t pc) const;
    void step_over_breakpoint();

    // Command handlers
    void handle(const std::string& cmd);
    void set_breakpoint_cmd(const std::string &address);

    // Other helpers
    void wait_for_signal() const;
    static uintptr_t read_abs_load_addr(pid_t pid);
    static bool is_pie(const std::string& prog_name);
    void set_abs_load_addr_on_launch();
};


#endif //DEBUGGER_H
