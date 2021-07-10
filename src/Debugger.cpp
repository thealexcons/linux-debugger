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

constexpr bool DEBUG_MODE = true;

// Launch the process to be debugged (called by child)
void Debugger::launch_process(const char *prog_name, pid_t pid) {
    personality(ADDR_NO_RANDOMIZE);                 // Disable address space randomisation
    ptrace(PTRACE_TRACEME, pid, nullptr, nullptr);  // Trace the child process (pid = 0)
    execl(prog_name, prog_name, nullptr);                  // Start program
}

// Wait for any signal to be sent to the child process
void Debugger::wait_for_signal() {
    int wait_status;
    waitpid(_pid, &wait_status, 0); // wait for signal in the debuggee

    // Get the signal information and handle specific signal
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, _pid, nullptr, &info);

    switch (info.si_signo) {
        case SIGTRAP:
            handle_sigtrap(info);
            break;
        case SIGSEGV:
        {
            auto line_entry = _dwarf_ctx.get_line_from_pc(get_offset_pc());
            std::cout << "Oops, you got a segfault on line " << std::dec << line_entry->line << ":\n";
            _dwarf_ctx.print_source(line_entry->file->path, line_entry->line, 1);
            exit(EXIT_SUCCESS); // TODO: query to run again?
        }
        default:
            std::cout << "Process finished running.\n";
            exit(EXIT_SUCCESS); // TODO: not sure if this is correct, process may get any signal (like a pipe)
    }
}

// Handle a SIGTRAP (due to a breakpoint or single stepping)
void Debugger::handle_sigtrap(siginfo_t info) {
    switch (info.si_code) {
        // Breakpoint is hit (either of the following codes)
        case SI_KERNEL:
        case TRAP_BRKPT:
        {
            set_pc(get_pc() - 1);   // Go back one instruction to execute the original instruction next
            auto rel_addr = get_offset_pc();
            std::cout << "Hit breakpoint at 0x" << std::hex << rel_addr << std::endl;
            print_source_lines(rel_addr, 1);
            return;
        }
        // Single stepping (do nothing)
        case TRAP_TRACE:
            return;
        default:;   // Ignore unknown SIGTRAPs
    }
}


// Read load address at launch (done here to avoid race conditions)
inline void Debugger::init_abs_load_addr_on_launch() {
    if (_abs_load_addr == UINTPTR_MAX) {
        /* If the program is compiled as PIE (by default), we need to read the abs load address to use relative addresses
        given by objdump. If PIE is turned off, objdump gives the absolute addresses, so set the offset to 0. */
        _abs_load_addr = is_elf_pie(_prog_name.c_str()) ? read_abs_load_addr(_pid) : 0;

        if constexpr(DEBUG_MODE) {
            std::cout << "(DEBUGGING) Process " << _pid << " loaded at 0x" << std::hex << _abs_load_addr
                      << (_abs_load_addr == 0 ? "(pie off)" : "(pie on)") << '\n';
        }
    }
}

// Run the debugger
void Debugger::run() {
    // Wait until signal is sent to the child (at launch or by software interrupt)
    wait_for_signal();
    init_abs_load_addr_on_launch(); // Only has effect once: on launch of child process

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
        continue_execution();
    } else if (is_prefixed_by(cmd, "break")) {
        set_breakpoint_cmd(args[1]);
    } else if (is_prefixed_by(cmd, "stepi")) {
        single_step_instruction();
        std::cout << "Stepped over one instruction.\n";
        print_source_lines(get_offset_pc());
    } else if (is_prefixed_by(cmd, "stepl")) {
        step_in();
        std::cout << "Stepped into line.\n";
        print_source_lines(get_offset_pc());
    } else if (is_prefixed_by(cmd, "next")) {
        step_over();
        std::cout << "Stepped over one line.\n";
        print_source_lines(get_offset_pc());
    } else if (is_prefixed_by(cmd, "finish")) {
        step_out();
        std::cout << "Stepped until end of function.\n";
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
void Debugger::continue_execution() {
    // Step over any possible breakpoint and continue execution
    single_step_instruction();
    ptrace(PTRACE_CONT, _pid, nullptr, nullptr);
    wait_for_signal();
}

// COMMAND: Set breakpoint
void Debugger::set_breakpoint_cmd(const std::string& address) {
    // TODO: more robust address parsing (same for memory/register writing)
    set_breakpoint(std::stol(address, nullptr, 16)); // assumes address is 0xADDR
}

// Sets (and enables) a breakpoint at an address
void Debugger::set_breakpoint(std::uintptr_t addr, bool print) {
    if (print) { std::cout << "Set breakpoint at address "; print_hex(addr); }
    Breakpoint bp {_pid, addr + _abs_load_addr};
    bp.enable();
    _breakpoints[addr] = bp;    // Index by relative address (without absolute load addr)
}

// Removes (and disables) a breakpoint
void Debugger::remove_breakpoint(std::uintptr_t addr, bool print) {
    if (_breakpoints.count(addr) != 0) {
        _breakpoints[addr].disable();
        _breakpoints.erase(addr);
        if (print) { std::cout << "Removed breakpoint at address "; print_hex(addr); }
    }
}

// Disables a breakpoint without removing it
void Debugger::disable_breakpoint(std::uintptr_t addr, bool print) {
    if (_breakpoints.count(addr) != 0) {
        _breakpoints[addr].disable();
        if (print) { std::cout << "Disabled breakpoint at address "; print_hex(addr); }
    }
}

// COMMAND: Memory read
uint64_t Debugger::read_memory(uint64_t addr) const {
    return ptrace(PTRACE_PEEKDATA, _pid, addr + _abs_load_addr, nullptr);
}

// COMMAND: Memory write
uint64_t Debugger::write_memory(uint64_t addr, uint8_t val) const {
    return ptrace(PTRACE_POKEDATA, _pid, addr + _abs_load_addr, val);
}

// Print the values of the registers
void Debugger::print_registers() const {
    for (int i = 0; i < NUM_REGS; i++) {
        Reg r = static_cast<Reg>(i);
        std::cout << get_reg_name(r) << " ";
        print_hex(get_reg_value(_pid, r), true);
    }
}

// Print the source line(s), given the relative address
void Debugger::print_source_lines(uint64_t addr, uint line_win_size) {
    try {
        auto line_entry = _dwarf_ctx.get_line_from_pc(addr);   // DWARF stores relative addresses
        _dwarf_ctx.print_source(line_entry->file->path, line_entry->line, line_win_size);
    } catch (const std::out_of_range& oor) {}
}

// Get the program counter (rip)
uint64_t Debugger::get_pc() const {
    return get_reg_value(_pid, Reg::rip);
}

// Set the program counter (rip)
void Debugger::set_pc(uint64_t pc) const {
    set_reg_value(_pid, Reg::rip, pc);
}

// Helper function to get the PC relative to the load address
inline uint64_t Debugger::get_offset_pc() {
    return get_pc() - _abs_load_addr;
}

// Perform a single step over an instruction via ptrace
void Debugger::single_step() {
    ptrace(PTRACE_SINGLESTEP, _pid, nullptr, nullptr);
    wait_for_signal();
}

// Single step over a (possible) breakpoint when resuming execution
void Debugger::single_step_instruction() {
    auto rel_addr = get_offset_pc();

    if (_breakpoints.count(rel_addr) != 0) {    // Check if the current instr is a breakpoint
        auto& bp = _breakpoints[rel_addr];
        if (bp.is_enabled()) {
            // Restore original instruction at breakpoint address
            bp.disable();
            // Single step over the breakpoint and re-enable it
            single_step();
            bp.enable();
        }
    } else {
        single_step();  // No breakpoint, just single step as usual
    }
}

// Step out of a function
void Debugger::step_out() {
    // Get the return address of the function, which is at 8 bytes from the frame pointer
    auto fp = get_reg_value(_pid, Reg::rbp);
    auto ret_addr = read_memory(fp + RET_ADDR_FRAME_OFFSET);

    // Set a temporary breakpoint at the return address of a function if it does not already exist
    bool remove_tmp_bp = false;
    if (_breakpoints.count(ret_addr - _abs_load_addr) == 0) {
        set_breakpoint(ret_addr, false);
        remove_tmp_bp = true;
    }

    // Continue execution until end of function
    continue_execution();
    if (remove_tmp_bp) {
        remove_breakpoint(ret_addr, false);
    }
};

// Step until we reach the next line of source code
void Debugger::step_in() {
    // Step through assembly representing the current line of source code
    auto line = _dwarf_ctx.get_line_from_pc(get_offset_pc())->line;
    while (_dwarf_ctx.get_line_from_pc(get_offset_pc())->line == line) {
        single_step_instruction();
    }

    // Print the next line
    auto line_entry = _dwarf_ctx.get_line_from_pc(get_offset_pc());
    _dwarf_ctx.print_source(line_entry->file->path, line_entry->line);
}


void Debugger::step_over() {
    auto func = _dwarf_ctx.get_function_from_pc(get_offset_pc());
    auto func_entry = _dwarf_ctx.get_func_entry(func);
    auto func_end = _dwarf_ctx.get_func_end(func);

    auto line = _dwarf_ctx.get_line_from_pc(func_entry);    // addr of func entry line
    auto curr_line = _dwarf_ctx.get_line_from_pc(get_offset_pc());  // addr of current line

    std::vector<std::uintptr_t> tmp_bps{};  // temporary list of breakpoints to be removed later

    // Loop through line table entries, checking that it's not the current line
    while (line->address < func_end) {
        auto abs_addr = line->address + _abs_load_addr;
        if (line->address != curr_line->address && _breakpoints.count(abs_addr) == 0) {
            set_breakpoint(abs_addr, false);
            tmp_bps.push_back(abs_addr);
        }
        ++line;
    }

    // Set breakpoint at the return address, similar to the step_out() method
    auto fp = get_reg_value(_pid, Reg::rbp);
    auto ret_addr = read_memory(fp + RET_ADDR_FRAME_OFFSET);
    if (_breakpoints.count(ret_addr) == 0) {
        set_breakpoint(ret_addr);
        tmp_bps.push_back(ret_addr);
    }

    continue_execution();
    for (auto& addr : tmp_bps) {
        remove_breakpoint(addr, false);
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
