#pragma once

class Command;

class Process {
public:
    Process();
    ~Process();

    int run(const Command& command) const;
};
