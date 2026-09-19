#include "Shell.hpp"

#include "Parser.hpp"
#include "Terminal.hpp"

#include <iostream>
#include <string>

Shell::Shell()
    : parser_(new Parser()),
      terminal_(new Terminal()) {
}

Shell::~Shell() {
    delete parser_;
    delete terminal_;
}

void Shell::run() {
    terminal_->printWelcome();

    while (true) {
        const std::string line = terminal_->readLine();

        if (line.empty()) {
            continue;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        executeLine(line);
    }
}

void Shell::executeLine(const std::string& line) {
    const auto tokens = parser_->parse(line);
    if (tokens.empty()) {
        return;
    }

    const int status = parser_->execute(tokens);
    if (status != 0) {
        std::cerr << "PelagiaShell: command failed with exit code " << status << std::endl;
    }
}
