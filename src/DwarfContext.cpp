//
// Created by alexcons on 25/05/2021.
//
#include "DwarfContext.h"
#include <fstream>
#include <iostream>
#include "Utils.h"


// Gets the DIE of the enclosing function from the current PC
// TODO extension: member functions and inlining (tut5)
dwarf::die DwarfContext::get_function_from_pc(uint64_t pc) const {
    for (auto& cu : _dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            for (const auto& die : cu.root()) {
                if (die.tag == dwarf::DW_TAG::subprogram) {
                    if (die_pc_range(die).contains(pc)) {
                        return die;
                    }
                }
            }
        }
    }
    throw std::out_of_range{"Cannot find enclosing function"};
}

// Get function entry address
uint64_t DwarfContext::get_func_entry(const dwarf::die& d) {
    return dwarf::at_low_pc(d);
}

// Get function end address
uint64_t DwarfContext::get_func_end(const dwarf::die& d) {
    return dwarf::at_high_pc(d);
}

// Get function from its name
 uint64_t DwarfContext::get_function_by_name(const std::string& name) const {
    for (const auto& cu : _dwarf.compilation_units()) {
        for (const auto& die : cu.root()) {
            if (die.has(dwarf::DW_AT::name) && dwarf::at_name(die) == name) {
                auto entry = get_line_from_pc(dwarf::at_low_pc(die));
                ++entry;    // skip prologue instructions (setting up call stack)
                return entry->address;
            }
        }
    }
}


// Gets the line corresponding to the current PC (which is a relative address)
dwarf::line_table::iterator DwarfContext::get_line_from_pc(uint64_t pc) const {
    for (auto& cu : _dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            auto& line_table = cu.get_line_table();
            auto iter = line_table.find_address(pc);

            if (iter == line_table.end()) {
                throw std::out_of_range{"Cannot find line entry"};
            }
            return iter;
        }
    }
    throw std::out_of_range{"Cannot find line entry"};
}

// Gets address for a particular line of a source file
uint64_t DwarfContext::get_source_line(const std::string& filename, uint line) {
    for (const auto& cu : _dwarf.compilation_units()) {
        if (Utils::is_suffixed_by(filename, dwarf::at_name(cu.root()))) {
            for (const auto& entry : cu.get_line_table()) {
                // Check that entry is the start of a statement
                if (entry.is_stmt && entry.line == line) {
                    return entry.address;
                }
            }
        }
    }
    throw std::invalid_argument{"Cannot find line in source file"};
}

// Prints the source lines
// TODO: move file ifstream init to constructor?
void DwarfContext::print_source(const std::string &file_name, uint line, uint num_lines) const {
    std::ifstream file {file_name};

    auto start_line = line <= num_lines ? 1 : line - num_lines;
    auto end_line = line + num_lines + (line < num_lines ? num_lines - line : 0) + 1;

    char c;
    uint curr_line = 1;
    while (curr_line != start_line && file.get(c)) {
        if (c == '\n') {
            curr_line++;
        }
    }

    // Display the cursor if we're at the line
    std::cout << (curr_line == line ? "> " : " ");

    // Write lines from start to end
    while (curr_line <= end_line && file.get(c)) {
        std::cout << c;
        if (c == '\n') {
            curr_line++;
            std::cout << (curr_line == line ? "> " : " ");
        }
    }
    std::cout << std::endl; // Flush the stream
}

// Map ELF enum types to our enum (to avoid dependency issues)
DwarfContext::SymbolType DwarfContext::get_symbol_type(elf::stt symbol) {
    switch (symbol) {
        case elf::stt::notype: return SymbolType::NoType;
        case elf::stt::object: return SymbolType::Object;
        case elf::stt::func: return SymbolType::Function;
        case elf::stt::section: return SymbolType::Section;
        case elf::stt::file: return SymbolType::File;
        default: throw std::invalid_argument{"Not supported"};
    }
}

// Lookup a symbol in the ELF information
// TODO build up map of name -> symbol to avoid repeated lookups
std::vector<DwarfContext::Symbol> DwarfContext::lookup_symbol(const std::string &name) {
    std::vector<Symbol> symbols{};

    for (const auto& section : _elf.sections()) {
        if (section.get_hdr().type != elf::sht::symtab && section.get_hdr().type != elf::sht::dynsym) {
            continue;
        }

        for (auto sym : section.as_symtab()) {
            if (sym.get_name() == name) {
                auto& data = sym.get_data();
                symbols.push_back(Symbol{get_symbol_type(data.type()), sym.get_name(), data.value});
            }
        }
    }

    return symbols;
}

