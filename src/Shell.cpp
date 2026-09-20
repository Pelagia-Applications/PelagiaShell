#include "Shell.hpp"

#include "Parser.hpp"
#include "Terminal.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string trimPipeline(const std::string& input) {
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

std::vector<std::string> splitPipeline(const std::string& line) {
    std::vector<std::string> parts;
    std::string current;
    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;

    for (char ch : line) {
        if (ch == '"' && !inSingleQuotes) {
            inDoubleQuotes = !inDoubleQuotes;
            current.push_back(ch);
            continue;
        }

        if (ch == '\'' && !inDoubleQuotes) {
            inSingleQuotes = !inSingleQuotes;
            current.push_back(ch);
            continue;
        }

        if (ch == '|' && !inDoubleQuotes && !inSingleQuotes) {
            const std::string trimmed = trimPipeline(current);
            if (!trimmed.empty()) {
                parts.push_back(trimmed);
            }
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    const std::string trimmed = trimPipeline(current);
    if (!trimmed.empty()) {
        parts.push_back(trimmed);
    }

    return parts;
}

std::string captureCommandOutput(const Parser& parser, const std::vector<std::string>& tokens) {
    std::ostringstream buffer;
    std::streambuf* original = std::cout.rdbuf(buffer.rdbuf());
    const int status = parser.execute(tokens);
    std::cout.flush();
    std::cout.rdbuf(original);

    if (status != 0) {
        return "";
    }

    std::string output = buffer.str();
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}
}

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
    const std::vector<std::string> stages = splitPipeline(line);
    if (stages.empty()) {
        return;
    }

    if (stages.size() == 1) {
        const auto tokens = parser_->parse(stages.front());
        if (tokens.empty()) {
            return;
        }

        const int status = parser_->execute(tokens);
        if (status != 0) {
            std::cerr << "PelagiaShell: command failed with exit code " << status << std::endl;
        }
        return;
    }

    std::string pipeOutput;
    for (std::size_t i = 0; i < stages.size(); ++i) {
        std::vector<std::string> tokens = parser_->parse(stages[i]);
        if (tokens.empty()) {
            continue;
        }

        if (!pipeOutput.empty() && i > 0) {
            tokens.push_back(pipeOutput);
        }

        const std::string stageOutput = captureCommandOutput(*parser_, tokens);
        if (i + 1 < stages.size()) {
            pipeOutput = stageOutput;
        } else if (!stageOutput.empty()) {
            std::cout << stageOutput << '\n';
        }
    }
}
