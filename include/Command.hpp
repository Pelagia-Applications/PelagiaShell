#pragma once

#include <string>
#include <vector>

class Command {
public:
    Command();
    explicit Command(const std::vector<std::string>& tokens);
    ~Command();

    const std::string& name() const;
    const std::vector<std::string>& arguments() const;
    bool isBuiltin() const;
    int execute() const;

private:
    std::string name_;
    std::vector<std::string> arguments_;
};
