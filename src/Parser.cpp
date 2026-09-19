#include "Parser.hpp"

#include "Command.hpp"
#include "Process.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Parser::Parser() = default;

Parser::~Parser() = default;

std::vector<std::string> Parser::parse(const std::string& input) const {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

int Parser::execute(const std::vector<std::string>& tokens) const {
    if (tokens.empty()) {
        return 0;
    }

    Command command(tokens);
    return Process().run(command);
}
