#include "Terminal.hpp"
#include "Theme.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

Terminal::Terminal() = default;

Terminal::~Terminal() = default;

void clearScreen() {
    std::cout << "\033[H\033[2J" << std::flush;
}

void Terminal::printWelcome() const {
    #define RESET   "\033[0m"
    #define GOLD    "\033[38;5;220m"
    #define WHITE   "\033[37m"
    #define GRAY    "\033[38;5;244m"

    // --- FRAME 1: COMPLETELY CLOSED ---
    clearScreen();
    std::cout << GRAY  << "            _.-'''-._" << std::endl;
    std::cout << GRAY  << "          .'         '-." << std::endl;
    std::cout << GRAY  << "        .'               '." << std::endl;
    std::cout << GRAY  << "       /                  \\" << std::endl;
    std::cout << GRAY  << "      /                    \\" << std::endl;
    std::cout << GRAY  << "     |                      |" << std::endl;
    std::cout << GRAY  << "     (__                  __)" << std::endl;
    std::cout << GRAY  << "      \\                    /" << std::endl;
    std::cout << GRAY  << "       '=================='" << std::endl;
    std::cout << GRAY  << "        \\                /" << std::endl;
    std::cout << GRAY  << "         '--___ ___--'" << std::endl;
    std::cout << GRAY  << "               -" << RESET << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // --- FRAME 2: SLIGHT PEEK (LID LIFTS) ---
    clearScreen();
    std::cout << GRAY  << "            _.-'''-._" << std::endl;
    std::cout << GRAY  << "          .'         '-." << std::endl;
    std::cout << GRAY  << "        .'               '." << std::endl;
    std::cout << GRAY  << "       /    " << GOLD << "__________" << GRAY  << "      \\" << std::endl;
    std::cout << GRAY  << "      /    " << GOLD << "/          \\" << GRAY  << "      \\" << std::endl;
    std::cout << GRAY  << "     |     " << GOLD << "|   " << WHITE << "(@@)" << GOLD << "   |" << GRAY  << "       |" << std::endl;
    std::cout << GRAY  << "     (__   " << GOLD << "'--______--'" << GRAY  << "      __)" << std::endl;
    std::cout << GRAY  << "      \\                    /" << std::endl;
    std::cout << GRAY  << "       '=================='" << std::endl;
    std::cout << GRAY  << "        \\                /" << std::endl;
    std::cout << GRAY  << "         '--___ ___--'" << std::endl;
    std::cout << GRAY  << "               -" << RESET << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // --- FRAME 3: FULLY OPEN ---
    clearScreen();
    std::cout << GRAY  << "            _.-'''-._" << std::endl;
    std::cout << GRAY  << "          .'         '-." << std::endl;
    std::cout << GRAY  << "        .'   " << GOLD << ".-----.     " << GRAY  << "'." << std::endl;
    std::cout << GRAY  << "       /    " << GOLD << "/       \\      " << GRAY  << "\\" << std::endl;
    std::cout << GRAY  << "      /    " << GOLD << "/  .---.  \\      " << GRAY  << "\\" << std::endl;
    std::cout << GRAY  << "     |     " << GOLD << "| /  " << WHITE << "()" << GOLD << "  \\ |       " << GRAY  << "|" << std::endl;
    std::cout << GRAY  << "     (__   " << GOLD << "||  " << WHITE << "(@@)" << GOLD << "  || " << GRAY  << "      __)" << std::endl;
    std::cout << GRAY  << "      \\    " << GOLD << "| \\  " << WHITE << "() " << GOLD << " / |" << GRAY  << "     /" << std::endl;
    std::cout << GRAY  << "       '==-" << GOLD << "'--___--'" << GRAY  << "-===='" << std::endl;
    std::cout << GRAY  << "        \\                /" << std::endl;
    std::cout << GRAY  << "         '--___ ___--'" << std::endl;
    std::cout << GRAY  << "               -" << RESET << std::endl;
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
