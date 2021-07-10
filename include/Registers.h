//
// Created by alexcons on 22/05/2021.
//

#ifndef REGISTERS_H
#define REGISTERS_H

#include <sys/user.h>
#include <algorithm>

// x86-64 registers (64 bit, integral value registers)
enum class Reg {
    rax,
    rbx,
    rcx,
    rdx,
    rdi,
    rsi,
    rbp,
    rsp,
    r8, r9, r10, r11, r12, r13, r14, r15,
    rip,
    rflags,
    cs,
    orig_rax,
    fs_base,
    gs_base,
    fs, gs, ss, ds, es
};

const std::size_t NUM_REGS = 27;

struct RegDescriptor {
    Reg r;
    int dwarf_r;
    std::string name;
};

// Must be in the same order as in sys/user.h
const std::array<RegDescriptor, NUM_REGS> global_reg_descriptors {{
    { Reg::r15, 15, "r15" },
    { Reg::r14, 14, "r14" },
    { Reg::r13, 13, "r13" },
    { Reg::r12, 12, "r12" },
    { Reg::rbp, 6, "rbp" },
    { Reg::rbx, 3, "rbx" },
    { Reg::r11, 11, "r11" },
    { Reg::r10, 10, "r10" },
    { Reg::r9, 9, "r9" },
    { Reg::r8, 8, "r8" },
    { Reg::rax, 0, "rax" },
    { Reg::rcx, 2, "rcx" },
    { Reg::rdx, 1, "rdx" },
    { Reg::rsi, 4, "rsi" },
    { Reg::rdi, 5, "rdi" },
    { Reg::orig_rax, -1, "orig_rax" },
    { Reg::rip, -1, "rip" },
    { Reg::cs, 51, "cs" },
    { Reg::rflags, 49, "eflags" },
    { Reg::rsp, 7, "rsp" },
    { Reg::ss, 52, "ss" },
    { Reg::fs_base, 58, "fs_base" },
    { Reg::gs_base, 59, "gs_base" },
    { Reg::ds, 53, "ds" },
    { Reg::es, 50, "es" },
    { Reg::fs, 54, "fs" },
    { Reg::gs, 55, "gs" },
}};


// Read a register in a process
uint64_t get_reg_value(pid_t pid, Reg r) {
    user_regs_struct regs{};
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

    // Find the request register in the regs struct and read its value
    auto iter = std::find_if(global_reg_descriptors.begin(), global_reg_descriptors.end(),
                 [r](auto&& rd) { return rd.r == r; });
    return *(reinterpret_cast<uint64_t*>(&regs) + (iter - global_reg_descriptors.begin()));
}

// Set a register in a process
void set_reg_value(pid_t pid, Reg r, uint64_t value) {
    user_regs_struct regs{};
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

    // Find the request register in the regs struct and read its value
    auto iter = std::find_if(global_reg_descriptors.begin(), global_reg_descriptors.end(),
                             [r](auto&& rd) { return rd.r == r; });

    if (iter == global_reg_descriptors.end()) {
        // could not find register? should not be reached if we validate the reg name
        throw std::out_of_range{"Unexpected register"};
    }

    // Write the value into the appropriate reg in the regs struct and update via ptrace call
    *(reinterpret_cast<uint64_t*>(&regs) + (iter - global_reg_descriptors.begin())) = value;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

// Read a register from its DWARF number (validate it first)
uint64_t get_reg_value_from_dwarf(pid_t pid, uint dwarf_num) {
    auto iter = std::find_if(global_reg_descriptors.begin(), global_reg_descriptors.end(),
                             [dwarf_num](auto&& rd){ return rd.dwarf_r == dwarf_num; });
    if (iter == global_reg_descriptors.end()) {
        throw std::out_of_range{"Unknown DWARF register number"};
    }

    return get_reg_value(pid, iter->r);
}

// Get the name of a Reg
std::string get_reg_name(Reg r) {
    auto iter = std::find_if(global_reg_descriptors.begin(), global_reg_descriptors.end(),
                             [r](auto&& rd){ return rd.r == r; });
    return iter->name;
}

// Get the Reg struct from its name
Reg get_reg_from_name(const std::string& name) {
    auto iter = std::find_if(global_reg_descriptors.begin(), global_reg_descriptors.end(),
                             [name](auto&& rd){ return rd.name == name; });
    return iter->r;
}


#endif //REGISTERS_H
