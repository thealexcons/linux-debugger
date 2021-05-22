//
// Created by alexcons on 22/05/2021.
//

#include <sys/ptrace.h>
#include "Breakpoint.h"

// Enable the breakpoint by replacing the byte at the address with INT3
void Breakpoint::enable() {
    if (!_enabled) {
        // Read byte at the given address of the debuggee process' memory
        auto data = ptrace(PTRACE_PEEKDATA, _pid, _addr, nullptr);
        _saved_byte = static_cast<uint8_t>(data & 0xff);  // Only want the bottom byte

        // Write int3 instruction into process' memory at the breakpoint address
        ptrace(PTRACE_POKEDATA, _pid, _addr, (data & ~0xff) | BREAKPOINT_INT3);
        _enabled = true;
    }
}

// Disable the breakpoint by restoring the saved byte back at the address
void Breakpoint::disable() {
    if (_enabled) {
        // Read byte at the given address of the process' memory
        auto data = ptrace(PTRACE_PEEKDATA, _pid, _addr, nullptr);
        auto restored_byte = (data & ~0xff) | _saved_byte;

        // Write the saved byte back in place of the INT3 instruction
        ptrace(PTRACE_POKEDATA, _pid, _addr, restored_byte);
        _enabled = false;
    }
}
