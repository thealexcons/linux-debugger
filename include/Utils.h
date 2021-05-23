//
// Created by alexcons on 22/05/2021.
//

#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <iomanip>
#include <elf.h>

// Splits a string into a vector of strings given a delimiter char
std::vector<std::string> split_by(const std::string& str, char delim) {
    std::vector<std::string> out;
    std::stringstream ss {str};
    std::string item;

    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

// Checks if pre is a prefix of str
bool is_prefixed_by(const std::string& pre, const std::string& str) {
    if (pre.size() > str.size()) {
        return false;
    }
    return std::equal(pre.begin(), pre.end(), str.begin());
}

// Print number in hex. Set full to true for full 64 bits to be shown
void print_hex(const uint64_t& num, bool full = false, bool end_line = true) {
    std::cout << "0x" << std::setfill('0');
    if (full) std::cout << std::setw( 16);
    std::cout << std::hex << num;
    if (end_line) std::cout << '\n';
}

// Function in C that checks if an ELF file is a shared object (PIE) or an executable
bool is_elf_pie(const char* file) {
    Elf64_Ehdr header;
    FILE *f = fopen(file, "rb");
    if (f) {
        fread(&header, 1, sizeof(header), f);
        // No need to check if it is an ELF file (it is already in execution)
    }
    fclose(f);
    return header.e_type == ET_DYN; // type flag is set to position independent (dynamic)
}

#endif //UTILS_H
