//
// Created by alexcons on 25/05/2021.
//

#ifndef DWARFCONTEXT_H
#define DWARFCONTEXT_H

#include <fcntl.h>

#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

class DwarfContext {
public:
    DwarfContext() = default;

    explicit DwarfContext(const std::string& prog_name) {
        auto fd = open(prog_name.c_str(), O_RDONLY);
        _elf = elf::elf{elf::create_mmap_loader(fd)};
        _dwarf = dwarf::dwarf{dwarf::elf::create_loader(_elf)};
    }

    dwarf::die get_function_from_pc(uint64_t pc) const;
    dwarf::line_table::iterator get_line_from_pc(uint64_t pc) const;
    void print_source(const std::string& file, uint line, uint num_lines=2) const;

private:
    dwarf::dwarf _dwarf;
    elf::elf _elf;

};


#endif //DWARFCONTEXT_H
