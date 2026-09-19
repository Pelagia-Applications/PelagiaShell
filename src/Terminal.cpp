#include "Terminal.hpp"
#include "Theme.hpp"

#include <iostream>
#include <string>

Terminal::Terminal() = default;

Terminal::~Terminal() = default;

void Terminal::printWelcome() const {
    std::cout << "       _.-''|''-._\n";
    std::cout << "    .-'     |     `-.\n";
    std::cout << "  .'\\       |       /`.\n";
    std::cout << " .'  \\      |      /   `.\n";
    std::cout << " :    \\     |     /    :\n";
    std::cout << " \\     \\    |    /     /\n";
    std::cout << "  `\\    \\   |   /    /'\n";
    std::cout << "    `\\   \\  |  /   /'\n";
    std::cout << "      `\\  \\ | /  /'\n";
    std::cout << "     _.-`\\ \\|/ /'-._\n";
    std::cout << "    {_____`\\|/'_____}\n";
    std::cout << "           `-'\n";
    std::cout << "PelagiaShell // oceanic shell\n";
    std::cout << "Use 'theme sunset', 'theme neon', 'theme hacker', or 'motd <message>' to customize it.\n";
    std::cout << "Type 'help', 'about', or 'exit' to begin.\n\n";
}

std::string Terminal::readLine() const {
    const std::string promptColor = promptColorCode(currentTheme());
    const std::string inputColor = inputColorCode(currentTheme());

    std::cout << promptColor << "pelagia~> " << inputColor << std::flush;
    std::string line;

    if (!std::getline(std::cin, line)) {
        std::cout << "\033[0m";
        return "exit";
    }

    std::cout << "\033[0m";
    return line;
}
