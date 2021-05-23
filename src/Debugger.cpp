//
// Created by alexcons on 22/05/2021.
//
#include <sys/wait.h>
#include <linenoise.h>
#include <iostream>
#include <fstream>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/personality.h>

#include "Debugger.h"
#include "Utils.h"
#include "Registers.h"

// Launch the process to be debugged (called by child)
void Debugger::launch_process(const char *prog_name, pid_t pid) {
    personality(ADDR_NO_RANDOMIZE);                 // Disable address space randomisation
    ptrace(PTRACE_TRACEME, pid, nullptr, nullptr);  // Trace the child process (pid = 0)
    execl(prog_name, prog_name, nullptr);                  // Start program
}

// Wait for any signal to be sent to the process
void Debugger::wait_for_signal() const {
    int wait_status;
    waitpid(_pid, &wait_status, 0);
}


// Run the debugger
void Debugger::run() {
    // Wait until SIGTRAP signal is sent to the child (at launch or by software interrupt)
    wait_for_signal();

    // Use linenoise library to handle user input and keep a history of commands
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

    // TODO: check number of args, etc. MORE ROBUST COMMAND PARSING

    if (is_prefixed_by(cmd, "continue")) {
        continue_cmd();
    } else if (is_prefixed_by(cmd, "break")) {
        set_breakpoint_cmd(args[1]);
    } else if (is_prefixed_by(cmd, "registers")) {
        if (is_prefixed_by(args[1], "print")) {
            print_registers();
        } else if (is_prefixed_by(args[1], "read")) {
            auto val = get_reg_value(_pid, get_reg_from_name(args[2]));
            print_hex(val, true);
        } else if (is_prefixed_by(args[1], "write")) {
            auto val = std::stol(args[3], nullptr, 16);
            set_reg_value(_pid, get_reg_from_name(args[2]), val);
            std::cout << "Wrote value "; print_hex(val, false, false);
            std::cout << " to register " << args[2] << '\n';
        } else {
            std::cerr << "Usage: 'print', 'read <reg>' or 'write <reg> <val>'\n";
        }
    } else if (is_prefixed_by(cmd, "memory")) {
        auto addr = std::stol(args[2], nullptr, 16);
        if (is_prefixed_by(args[1], "read")) {
            print_hex(read_memory(addr));
        } else if (is_prefixed_by(args[1], "write")) {
            auto val = std::stol(args[3], nullptr, 16);
            write_memory(addr, val);
        } else {
            std::cerr << "Usage: 'print', 'read <reg>' or 'write <reg> <val>'\n";
        }
    } else {
        std::cerr << "Unknown command\n";
    }
}

// COMMAND: Continue execution
void Debugger::continue_cmd() {
    // Step over any possible breakpoint and continue execution
    step_over_breakpoint();
    ptrace(PTRACE_CONT, _pid, nullptr, nullptr);
    wait_for_signal();
}

// COMMAND: Set breakpoint
void Debugger::set_breakpoint_cmd(const std::string& address) {
    // TODO: more robust address parsing (same for memory/register writing)
    set_breakpoint(std::stol(address, nullptr, 16)); // assumes address is 0xADDR
}

// Sets (and enables) a breakpoint at an address
void Debugger::set_breakpoint(std::uintptr_t addr) {
    std::cout << "Set breakpoint at address ";
    print_hex(addr);
    Breakpoint bp {_pid, addr + _abs_load_addr};
    bp.enable();
    _breakpoints[addr] = bp;    // Index by relative address (without absolute load addr)
}

// Removes a breakpoint
void Debugger::remove_breakpoint(std::uintptr_t addr) {
    if (_breakpoints.count(addr) != 0) {
        _breakpoints[addr].disable();
        _breakpoints.erase(addr);
        std::cout << "Removed breakpoint at address ";
        print_hex(addr);
    }
}

// Disables a breakpoint
void Debugger::disable_breakpoint(std::uintptr_t addr) {
    if (_breakpoints.count(addr) != 0) {
        _breakpoints[addr].disable();
        std::cout << "Disabled breakpoint at address ";
        print_hex(addr);
    }
}

// COMMAND: Memory read
uint64_t Debugger::read_memory(uint64_t addr) const {
    return ptrace(PTRACE_PEEKDATA, _pid, addr, nullptr);
}

// COMMAND: Memory write
uint64_t Debugger::write_memory(uint64_t addr, uint8_t val) const {
    return ptrace(PTRACE_POKEDATA, _pid, addr, val);
}

// Print the values of the registers
void Debugger::print_registers() const {
    for (int i = 0; i < NUM_REGS; i++) {
        Reg r = static_cast<Reg>(i);
        std::cout << get_reg_name(r) << " ";
        print_hex(get_reg_value(_pid, r), true);
    }
}

// Get the program counter (rip)
uint64_t Debugger::get_pc() const {
    return get_reg_value(_pid, Reg::rip);
}

// Set the program counter (rip)
void Debugger::set_pc(uint64_t pc) const {
    set_reg_value(_pid, Reg::rip, pc);
}

// Step over a (possible) breakpoint when resuming execution
void Debugger::step_over_breakpoint() {
    auto possible_bp_addr = get_pc() - 1;   // Possible breakpoint is the current instr (one less than pc)
    auto rel_bp_addr = possible_bp_addr - _abs_load_addr;   // Index into map is the relative address

    if (_breakpoints.count(rel_bp_addr) != 0) {    // Check if the current instr is a breakpoint
        auto& bp = _breakpoints[rel_bp_addr];
        if (bp.is_enabled()) {
            // Put execution before breakpoint and disable it (restoring original instr)
            set_pc(possible_bp_addr);
            bp.disable();

            // Single step over the breakpoint and re-enable it
            ptrace(PTRACE_SINGLESTEP, _pid, nullptr, nullptr);
            wait_for_signal();
            bp.enable();
        }
    }
}

// Get the absolute load address of the child process from /proc/<pid>/maps
uintptr_t Debugger::read_abs_load_addr(pid_t pid) {
    std::string path = "/proc/";
    path += std::to_string(pid);
    path += "/maps";

    std::ifstream ifs {path};
    std::string addr_str;
    std::getline(ifs, addr_str, '-');   // Read first address in file (number before the first dash)

    std::stringstream ss;
    uintptr_t addr;
    ss << std::hex << addr_str;
    ss >> addr;

    return addr;
}

// Checks if the binary was compiled as a position independent executable or not
bool Debugger::is_pie(const std::string& prog_name) {
    return is_elf_pie(prog_name.c_str());
}

