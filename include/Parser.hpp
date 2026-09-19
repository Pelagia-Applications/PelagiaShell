#pragma once

#include <string>
#include <vector>

class Parser {
public:
    Parser();
    ~Parser();

    std::vector<std::string> parse(const std::string& input) const;
    int execute(const std::vector<std::string>& tokens) const;
};
