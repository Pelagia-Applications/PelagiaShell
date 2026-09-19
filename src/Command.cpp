#include "Command.hpp"
#include "Theme.hpp"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <ctime>
#else
#include <unistd.h>
#endif

namespace {
std::string joinArguments(const std::vector<std::string>& arguments) {
    std::string result;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        result += arguments[i] + (i + 1 < arguments.size() ? " " : "");
    }
    return result;
}

std::string base64Encode(const std::string& input) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    for (std::size_t i = 0; i < input.size(); i += 3) {
        const unsigned char a = static_cast<unsigned char>(input[i]);
        const unsigned char b = i + 1 < input.size() ? static_cast<unsigned char>(input[i + 1]) : 0;
        const unsigned char c = i + 2 < input.size() ? static_cast<unsigned char>(input[i + 2]) : 0;

        const unsigned char first = static_cast<unsigned char>((a >> 2) & 0x3F);
        const unsigned char second = static_cast<unsigned char>(((a & 0x03) << 4) | ((b >> 4) & 0x0F));
        const unsigned char third = static_cast<unsigned char>(((b & 0x0F) << 2) | ((c >> 6) & 0x03));
        const unsigned char fourth = static_cast<unsigned char>(c & 0x3F);

        output.push_back(chars[first]);
        output.push_back(chars[second]);
        output.push_back((i + 1 < input.size()) ? chars[third] : '=');
        output.push_back((i + 2 < input.size()) ? chars[fourth] : '=');
    }

    return output;
}

std::string base64Decode(const std::string& input) {
    std::string cleaned;
    cleaned.reserve(input.size());
    for (char ch : input) {
        if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') {
            continue;
        }
        cleaned.push_back(ch);
    }

    if (cleaned.empty()) {
        return "";
    }

    std::string output;
    output.reserve(cleaned.size() * 3 / 4);

    std::vector<int> values(cleaned.size());
    for (std::size_t i = 0; i < cleaned.size(); ++i) {
        char ch = cleaned[i];
        if (ch >= 'A' && ch <= 'Z') {
            values[i] = ch - 'A';
        } else if (ch >= 'a' && ch <= 'z') {
            values[i] = ch - 'a' + 26;
        } else if (ch >= '0' && ch <= '9') {
            values[i] = ch - '0' + 52;
        } else if (ch == '+') {
            values[i] = 62;
        } else if (ch == '/') {
            values[i] = 63;
        } else if (ch == '=') {
            values[i] = 0;
        } else {
            throw std::invalid_argument("invalid base64 input");
        }
    }

    std::size_t index = 0;
    while (index < cleaned.size() && cleaned[index] == '=') {
        ++index;
    }

    for (std::size_t i = 0; i + 4 <= cleaned.size(); i += 4) {
        const int a = values[i];
        const int b = values[i + 1];
        const int c = values[i + 2];
        const int d = values[i + 3];

        output.push_back(static_cast<char>((a << 2) | (b >> 4)));
        if (cleaned[i + 2] != '=') {
            output.push_back(static_cast<char>(((b & 0x0F) << 4) | (c >> 2)));
        }
        if (cleaned[i + 3] != '=') {
            output.push_back(static_cast<char>(((c & 0x03) << 6) | d));
        }
    }

    if (cleaned.size() % 4 != 0) {
        throw std::invalid_argument("invalid base64 length");
    }

    return output;
}
}

Command::Command() = default;

Command::Command(const std::vector<std::string>& tokens)
    : name_(),
      arguments_() {
    if (!tokens.empty()) {
        name_ = tokens.front();
        arguments_.assign(tokens.begin() + 1, tokens.end());
    }
}

Command::~Command() = default;

const std::string& Command::name() const {
    return name_;
}

const std::vector<std::string>& Command::arguments() const {
    return arguments_;
}

bool Command::isBuiltin() const {
    return name_ == "echo" || name_ == "cd" || name_ == "pwd" || name_ == "ls" ||
           name_ == "help" || name_ == "about" || name_ == "theme" || name_ == "motd" ||
           name_ == "banner" || name_ == "mkdir" || name_ == "rmdir" ||
           name_ == "touch" || name_ == "cat" || name_ == "clear" || name_ == "date" ||
           name_ == "whoami" || name_ == "uname" || name_ == "rand" || name_ == "random" ||
           name_ == "b64encode" || name_ == "b64decode";
}

int Command::execute() const {
    if (name_.empty()) {
        return 0;
    }

    if (name_ == "echo") {
        for (std::size_t i = 0; i < arguments_.size(); ++i) {
            std::cout << arguments_[i] << (i + 1 < arguments_.size() ? " " : "");
        }
        std::cout << '\n';
        return 0;
    }

    if (name_ == "pwd") {
        std::cout << std::filesystem::current_path().string() << '\n';
        return 0;
    }

    if (name_ == "cd") {
        if (arguments_.empty()) {
            std::cerr << "Usage: cd <directory>\n";
            return 1;
        }

#ifdef _WIN32
        if (_chdir(arguments_[0].c_str()) != 0) {
#else
        if (chdir(arguments_[0].c_str()) != 0) {
#endif
            std::perror("cd");
            return 1;
        }
        return 0;
    }

    if (name_ == "ls") {
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
            std::cout << entry.path().filename().string() << '\n';
        }
        return 0;
    }

    if (name_ == "mkdir") {
        if (arguments_.empty()) {
            std::cerr << "Usage: mkdir <directory>\n";
            return 1;
        }

        for (const auto& path : arguments_) {
            std::error_code ec;
            if (!std::filesystem::create_directories(path, ec)) {
                if (ec) {
                    std::cerr << "mkdir: " << ec.message() << '\n';
                    return 1;
                }
            }
        }
        return 0;
    }

    if (name_ == "rmdir") {
        if (arguments_.empty()) {
            std::cerr << "Usage: rmdir <directory>\n";
            return 1;
        }

        for (const auto& path : arguments_) {
            std::error_code ec;
            if (!std::filesystem::remove_all(path, ec)) {
                if (ec) {
                    std::cerr << "rmdir: " << ec.message() << '\n';
                    return 1;
                }
                std::cerr << "rmdir: directory not found: " << path << '\n';
                return 1;
            }
        }
        return 0;
    }

    if (name_ == "touch") {
        if (arguments_.empty()) {
            std::cerr << "Usage: touch <file>\n";
            return 1;
        }

        for (const auto& path : arguments_) {
            std::ofstream file(path, std::ios::app);
            if (!file) {
                std::cerr << "touch: unable to create file: " << path << '\n';
                return 1;
            }
        }
        return 0;
    }

    if (name_ == "cat") {
        if (arguments_.empty()) {
            std::cerr << "Usage: cat <file>\n";
            return 1;
        }

        for (const auto& path : arguments_) {
            std::ifstream input(path);
            if (!input) {
                std::cerr << "cat: cannot open file: " << path << '\n';
                return 1;
            }

            std::string line;
            while (std::getline(input, line)) {
                std::cout << line << '\n';
            }
        }
        return 0;
    }

    if (name_ == "clear") {
        std::cout << "\033[2J\033[H" << std::flush;
        return 0;
    }

    if (name_ == "date") {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);

#ifdef _WIN32
        char buffer[26];
        if (ctime_s(buffer, sizeof(buffer), &time) == 0) {
            std::cout << buffer;
        } else {
            std::cout << "date: unable to read time\n";
        }
#else
        std::cout << std::asctime(std::localtime(&time));
#endif
        return 0;
    }

    if (name_ == "whoami") {
#ifdef _WIN32
        const char* user = std::getenv("USERNAME");
#else
        const char* user = std::getenv("USER");
#endif
        std::cout << (user ? user : "unknown") << '\n';
        return 0;
    }

    if (name_ == "uname") {
#ifdef _WIN32
        std::cout << "Windows\n";
#else
        std::cout << "Linux\n";
#endif
        return 0;
    }

    if (name_ == "about") {
        const std::string theme = currentTheme();
        std::cout << themeColorCode(theme) << themeTitle(theme) << "\033[0m\n";
        std::cout << themeColorCode(theme) << currentMotd() << "\033[0m\n";
        std::cout << "Current vibe: " << themeColorCode(theme) << themeLabel(theme) << "\033[0m\n";
        return 0;
    }

    if (name_ == "rand" || name_ == "random") {
        if (arguments_.size() != 2) {
            std::cerr << "Usage: rand <min> <max>\n";
            return 1;
        }

        try {
            const long long minValue = std::stoll(arguments_[0]);
            const long long maxValue = std::stoll(arguments_[1]);

            std::random_device rd;
            std::mt19937_64 generator(rd());
            std::uniform_int_distribution<long long> dist(minValue, maxValue);
            std::cout << dist(generator) << '\n';
            return 0;
        } catch (const std::exception&) {
            std::cerr << "Usage: rand <min> <max>\n";
            return 1;
        }
    }

    if (name_ == "b64encode") {
        if (arguments_.empty()) {
            std::cerr << "Usage: b64encode <text>\n";
            return 1;
        }

        const std::string value = joinArguments(arguments_);
        std::cout << base64Encode(value) << '\n';
        return 0;
    }

    if (name_ == "b64decode") {
        if (arguments_.empty()) {
            std::cerr << "Usage: b64decode <text>\n";
            return 1;
        }

        try {
            const std::string value = joinArguments(arguments_);
            const std::string decoded = base64Decode(value);
            std::cout << decoded << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "b64decode: " << ex.what() << '\n';
            return 1;
        }
    }

    if (name_ == "theme") {
        if (arguments_.empty()) {
            std::cout << "Current theme: " << themeLabel(currentTheme()) << '\n';
            std::cout << "Available themes: ocean, sunset, neon, hacker\n";
            return 0;
        }

        const std::string requested = arguments_[0];
        if (requested == "ocean" || requested == "sunset" || requested == "neon" || requested == "hacker") {
            currentTheme() = requested;
            std::cout << themeColorCode(requested) << "Theme set to: " << requested << "\033[0m\n";
            return 0;
        }

        std::cerr << "Unknown theme: " << requested << "\n";
        std::cerr << "Available themes: ocean, sunset, neon, hacker\n";
        return 1;
    }

    if (name_ == "motd" || name_ == "banner") {
        if (arguments_.empty()) {
            std::cout << currentMotd() << '\n';
            return 0;
        }

        std::string message;
        for (std::size_t i = 0; i < arguments_.size(); ++i) {
            message += arguments_[i] + (i + 1 < arguments_.size() ? " " : "");
        }
        currentMotd() = message;
        std::cout << "Motd updated.\n";
        return 0;
    }

    if (name_ == "help") {
        std::cout << "PelagiaShell builtins: echo, cd, pwd, ls, mkdir, rmdir, touch, cat, clear, date, whoami, uname, rand, random, b64encode, b64decode, about, theme, motd, banner, help, exit\n";
        return 0;
    }

    return 1;
}
