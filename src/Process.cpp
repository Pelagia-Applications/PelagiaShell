#include "Process.hpp"

#include "Command.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

Process::Process() = default;

Process::~Process() = default;

int Process::run(const Command& command) const {
    if (command.isBuiltin()) {
        return command.execute();
    }

    std::string fullCommand = command.name();
    for (const auto& arg : command.arguments()) {
        fullCommand += " " + arg;
    }

    return std::system(fullCommand.c_str());
}
