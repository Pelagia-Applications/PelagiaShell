#pragma once

#include <string>

class Parser;
class Terminal;

class Shell {
public:
    Shell();
    ~Shell();

    void run();

private:
    void executeLine(const std::string& line);

    Parser* parser_;
    Terminal* terminal_;
};
