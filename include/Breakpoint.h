//
// Created by alexcons on 22/05/2021.
//

#ifndef BREAKPOINT_H
#define BREAKPOINT_H


#include <csignal>
#include <cstdint>

// int 3 interrupt for x86 (triggers SIGTRAP)
const std::uintptr_t BREAKPOINT_INT3 = 0xcc;

class Breakpoint {
public:
    Breakpoint() = default;
    Breakpoint(pid_t pid, std::uintptr_t addr) : _pid{pid}, _addr{addr} {};

    void enable();
    void disable();

    bool is_enabled() const { return _enabled; }
    std::uintptr_t get_address() const { return _addr; }

private:
    pid_t _pid{};
    std::uintptr_t _addr{};
    bool _enabled = false;
    uint8_t _saved_byte{};    // Byte of data that used to be at the breakpoint address before replacing with interrupt
};


#endif //BREAKPOINT_H
