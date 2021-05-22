//
// Created by alexcons on 22/05/2021.
//

#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <sstream>

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

#endif //UTILS_H
