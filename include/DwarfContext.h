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

    uint64_t get_function_by_name(const std::string& name) const;
    uint64_t get_source_line(const std::string& filename, uint line);
    uint64_t get_func_entry(const dwarf::die &d);
    uint64_t get_func_end(const dwarf::die &d);

    enum class SymbolType {
        NoType,
        Object,
        Function,
        Section,
        File,
    };

    struct Symbol {
        SymbolType type;
        std::string name;
        std::uint64_t addr;
    };
    static SymbolType get_symbol_type(elf::stt symbol);
    std::vector<Symbol> lookup_symbol(const std::string& name);

private:
    dwarf::dwarf _dwarf;
    elf::elf _elf;
};

inline std::string to_string(DwarfContext::SymbolType st) {
    switch (st) {
        case DwarfContext::SymbolType::NoType: return "notype";
        case DwarfContext::SymbolType::Object: return "object";
        case DwarfContext::SymbolType::Function: return "func";
        case DwarfContext::SymbolType::Section: return "section";
        case DwarfContext::SymbolType::File: return "file";
    }
}


#endif //DWARFCONTEXT_H
