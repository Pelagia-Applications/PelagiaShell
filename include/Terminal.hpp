#pragma once

#include <string>

class Terminal {
public:
    Terminal();
    ~Terminal();

    void printWelcome() const;
    std::string readLine() const;
};
