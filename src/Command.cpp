#include "Command.hpp"
#include "Theme.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
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

std::uint32_t sha256RoTR(std::uint32_t value, std::uint32_t amount) {
    return (value >> amount) | (value << (32u - amount));
}

std::string sha256Hex(const std::string& input) {
    static const std::array<std::uint32_t, 64> k = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };

    std::array<std::uint32_t, 8> hash = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };

    std::vector<unsigned char> message(input.begin(), input.end());
    message.push_back(0x80u);

    while ((message.size() % 64u) != 56u) {
        message.push_back(0x00u);
    }

    const std::uint64_t bitLength = static_cast<std::uint64_t>(input.size()) * 8u;
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<unsigned char>((bitLength >> (static_cast<unsigned int>(i) * 8u)) & 0xFFu));
    }

    for (std::size_t chunkOffset = 0; chunkOffset < message.size(); chunkOffset += 64u) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16u; ++i) {
            const std::size_t idx = chunkOffset + i * 4u;
            w[i] = (static_cast<std::uint32_t>(message[idx]) << 24) |
                   (static_cast<std::uint32_t>(message[idx + 1]) << 16) |
                   (static_cast<std::uint32_t>(message[idx + 2]) << 8) |
                   static_cast<std::uint32_t>(message[idx + 3]);
        }

        for (std::size_t i = 16u; i < 64u; ++i) {
            const std::uint32_t s0 = sha256RoTR(w[i - 15u], 7u) ^ sha256RoTR(w[i - 15u], 18u) ^ (w[i - 15u] >> 3u);
            const std::uint32_t s1 = sha256RoTR(w[i - 2u], 17u) ^ sha256RoTR(w[i - 2u], 19u) ^ (w[i - 2u] >> 10u);
            w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
        }

        std::uint32_t a = hash[0];
        std::uint32_t b = hash[1];
        std::uint32_t c = hash[2];
        std::uint32_t d = hash[3];
        std::uint32_t e = hash[4];
        std::uint32_t f = hash[5];
        std::uint32_t g = hash[6];
        std::uint32_t h = hash[7];

        for (std::size_t i = 0; i < 64u; ++i) {
            const std::uint32_t s1 = sha256RoTR(e, 6u) ^ sha256RoTR(e, 11u) ^ sha256RoTR(e, 25u);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = sha256RoTR(a, 2u) ^ sha256RoTR(a, 13u) ^ sha256RoTR(a, 22u);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    std::ostringstream output;
    output << std::hex << std::nouppercase;
    for (std::uint32_t value : hash) {
        output << std::setw(8) << std::setfill('0') << value;
    }
    return output.str();
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
           name_ == "sha256" || name_ == "sha256file" || name_ == "b64encode" || name_ == "b64decode";
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

    if (name_ == "sha256" || name_ == "sha256file") {
        if (arguments_.empty()) {
            std::cerr << "Usage: sha256 <text|file>\n";
            return 1;
        }

        try {
            std::string value = joinArguments(arguments_);
            if (arguments_.size() == 1 && std::filesystem::exists(arguments_[0])) {
                std::ifstream input(arguments_[0], std::ios::binary);
                if (!input) {
                    std::cerr << "sha256: unable to open file: " << arguments_[0] << '\n';
                    return 1;
                }

                std::ostringstream buffer;
                buffer << input.rdbuf();
                value = buffer.str();
            }

            std::cout << sha256Hex(value) << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "sha256: " << ex.what() << '\n';
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
