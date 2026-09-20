#include "Command.hpp"
#include "Process.hpp"
#include "Theme.hpp"

#include <algorithm>
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
#include <thread>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <ctime>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {
std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return L"";
    }

    std::wstring result(static_cast<std::size_t>(size) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
    return result;
}

std::string joinArguments(const std::vector<std::string>& arguments) {
    std::string result;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        result += arguments[i] + (i + 1 < arguments.size() ? " " : "");
    }
    return result;
}

std::string escapeShellArgument(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size() + 2);
    escaped.push_back('"');

    for (const char ch : input) {
        if (ch == '"' || ch == '\\' || ch == '$' || ch == '`') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }

    escaped.push_back('"');
    return escaped;
}

std::string trim(const std::string& input) {
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

std::string stripAnsiCodes(const std::string& input) {
    std::string output;
    output.reserve(input.size());

    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\x1b' && i + 1 < input.size()) {
            if (input[i + 1] == '[') {
                i += 2;
                while (i < input.size() && (input[i] < '@' || input[i] > '~')) {
                    ++i;
                }
                if (i < input.size()) {
                    continue;
                }
                break;
            }
        }
        output.push_back(input[i]);
    }

    return output;
}

std::string normalizeWhitespace(const std::string& input) {
    std::string output;
    bool previousWasSpace = false;

    for (const char ch : input) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            if (!output.empty() && !previousWasSpace) {
                output.push_back(' ');
                previousWasSpace = true;
            }
        } else {
            output.push_back(ch);
            previousWasSpace = false;
        }
    }

    return trim(output);
}

std::string captureCommandOutput(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return "";
    }

    std::ostringstream captured;
    const std::streambuf* original = std::cout.rdbuf(captured.rdbuf());
    const int status = Process().run(Command(tokens));
    std::cout.flush();
    std::cout.rdbuf(const_cast<std::streambuf*>(original));

    if (status != 0) {
        return "";
    }

    std::string output = captured.str();
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return trim(output);
}

std::vector<std::string> resolveCommandChainArguments(const std::vector<std::string>& arguments, const std::string& currentName) {
    std::vector<std::string> resolved = arguments;
    for (std::size_t i = 0; i < resolved.size(); ++i) {
        const std::string& arg = resolved[i];
        if (arg.empty() || arg[0] == '"' || arg[0] == '\'') {
            continue;
        }

        const std::vector<std::string> candidateTokens = { arg };
        const Command candidate(candidateTokens);
        if (!candidate.isBuiltin() || candidate.name() == currentName) {
            continue;
        }

        const std::string substituted = captureCommandOutput(candidateTokens);
        if (!substituted.empty()) {
            resolved[i] = substituted;
        }
    }
    return resolved;
}

std::vector<std::string> splitDelimitedLine(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
            continue;
        }

        if (ch == delimiter && !inQuotes) {
            fields.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    fields.push_back(current);
    return fields;
}

std::vector<std::vector<std::string>> parseTableData(const std::string& input, char delimiter) {
    std::vector<std::vector<std::string>> rows;
    std::istringstream stream(input);
    std::string line;

    while (std::getline(stream, line)) {
        if (trim(line).empty()) {
            continue;
        }
        rows.push_back(splitDelimitedLine(line, delimiter));
    }

    return rows;
}

std::string truncateCell(const std::string& value, std::size_t width) {
    if (value.size() <= width) {
        return value;
    }

    if (width <= 3) {
        return value.substr(0, width);
    }

    return value.substr(0, width - 3) + "...";
}

std::string padCell(const std::string& value, std::size_t width) {
    std::string padded = value;
    if (padded.size() < width) {
        padded.append(width - padded.size(), ' ');
    }
    return padded;
}

std::string renderAnsiGrid(const std::vector<std::vector<std::string>>& rows, std::size_t maxColumns, std::size_t limit, bool colorEnabled) {
    if (rows.empty()) {
        return "(no data)\n";
    }

    std::vector<std::size_t> widths(maxColumns, 0);
    for (const auto& row : rows) {
        for (std::size_t col = 0; col < maxColumns; ++col) {
            const std::string cell = (col < row.size()) ? row[col] : "";
            widths[col] = std::max(widths[col], std::min<std::size_t>(std::max<std::size_t>(cell.size(), 1), 28u));
        }
    }

    const std::string reset = "\033[0m";
    const std::string blue = colorEnabled ? "\033[38;5;45m" : "";
    const std::string cyan = colorEnabled ? "\033[38;5;117m" : "";
    const std::string dim = colorEnabled ? "\033[38;5;245m" : "";
    const std::string accent = colorEnabled ? "\033[38;5;220m" : "";
    const std::string headerBg = colorEnabled ? "\033[48;5;24m" : "";

    std::ostringstream output;
    auto printBorder = [&](const char left, const char mid, const char right) {
        output << left;
        for (std::size_t col = 0; col < maxColumns; ++col) {
            output << std::string(widths[col] + 2, '-') << (col + 1 == maxColumns ? right : mid);
        }
        output << '\n';
    };

    printBorder('+', '+', '+');

    std::size_t visibleRows = 0;
    for (std::size_t rowIndex = 0; rowIndex < rows.size() && (limit == 0 || visibleRows < limit); ++rowIndex) {
        const auto& row = rows[rowIndex];
        const bool headerRow = rowIndex == 0;

        output << (headerRow ? (headerBg + blue) : (rowIndex % 2 == 0 ? dim : accent));
        output << '|';
        for (std::size_t col = 0; col < maxColumns; ++col) {
            const std::string value = (col < row.size()) ? row[col] : "";
            const std::string display = truncateCell(value, widths[col]);
            output << ' ' << padCell(display, widths[col]) << ' ' << '|';
        }
        output << reset << '\n';
        ++visibleRows;

        if (headerRow) {
            output << (headerBg + cyan);
            output << '|';
            for (std::size_t col = 0; col < maxColumns; ++col) {
                output << ' ' << padCell("", widths[col]) << ' ' << '|';
            }
            output << reset << '\n';
        }
    }

    printBorder('+', '+', '+');
    if (rows.size() > visibleRows) {
        output << "... truncated\n";
    }
    return output.str();
}

std::string commandOutput(const std::string& command) {
    std::string output;
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) {
        return "";
    }

    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

std::string firstMeaningfulLine(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string cleaned = normalizeWhitespace(line);
        if (!cleaned.empty() && cleaned != "Name" && cleaned != "Name " && cleaned != "VALUE" && cleaned != "TotalPhysicalMemory") {
            return cleaned;
        }
    }
    return "Unknown";
}

std::string formatBytesToGiB(std::uint64_t bytes) {
    const double gigabytes = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << gigabytes << " GiB";
    return stream.str();
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

std::string uuEncode(const std::string& input) {
    std::string output;
    output.reserve(input.size() * 2 + 8);

    for (std::size_t i = 0; i < input.size(); i += 3) {
        const std::size_t remaining = input.size() - i;
        const std::size_t chunkLen = std::min<std::size_t>(remaining, 3u);

        const unsigned char b0 = static_cast<unsigned char>(input[i]);
        const unsigned char b1 = chunkLen > 1 ? static_cast<unsigned char>(input[i + 1]) : 0;
        const unsigned char b2 = chunkLen > 2 ? static_cast<unsigned char>(input[i + 2]) : 0;

        const unsigned char a = static_cast<unsigned char>((b0 >> 2) & 0x3F) + 0x20;
        const unsigned char b = static_cast<unsigned char>(((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)) + 0x20;
        const unsigned char c = static_cast<unsigned char>(((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)) + 0x20;
        const unsigned char d = static_cast<unsigned char>(b2 & 0x3F) + 0x20;

        output.push_back(static_cast<char>(chunkLen + 0x20));
        output.push_back(static_cast<char>(a));
        output.push_back(static_cast<char>(b));
        output.push_back(static_cast<char>(c));
        output.push_back(static_cast<char>(d));
        output.push_back('\n');
    }

    output += "`\n";
    return output;
}

std::string uuDecode(const std::string& input) {
    std::string cleaned;
    cleaned.reserve(input.size());
    for (char ch : input) {
        if (ch == '\r' || ch == '\n') {
            continue;
        }
        cleaned.push_back(ch);
    }

    if (cleaned.empty()) {
        return "";
    }

    std::string output;
    for (std::size_t i = 0; i < cleaned.size(); i += 5) {
        if (i + 4 >= cleaned.size()) {
            break;
        }

        const unsigned char length = static_cast<unsigned char>(cleaned[i]) - 0x20;
        if (length == 0) {
            break;
        }

        const unsigned char a = static_cast<unsigned char>(cleaned[i + 1]) - 0x20;
        const unsigned char b = static_cast<unsigned char>(cleaned[i + 2]) - 0x20;
        const unsigned char c = static_cast<unsigned char>(cleaned[i + 3]) - 0x20;
        const unsigned char d = static_cast<unsigned char>(cleaned[i + 4]) - 0x20;

        const unsigned char byte0 = static_cast<unsigned char>((a << 2) | (b >> 4));
        output.push_back(static_cast<char>(byte0));

        if (length >= 2) {
            const unsigned char byte1 = static_cast<unsigned char>(((b & 0x0F) << 4) | (c >> 2));
            output.push_back(static_cast<char>(byte1));
        }

        if (length >= 3) {
            const unsigned char byte2 = static_cast<unsigned char>(((c & 0x03) << 6) | d);
            output.push_back(static_cast<char>(byte2));
        }
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
           name_ == "dice" || name_ == "coinflip" || name_ == "uud" || name_ == "weather" ||
           name_ == "specs" || name_ == "req" || name_ == "gridview" || name_ == "grid" || name_ == "gv" ||
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

    if (name_ == "dice") {
        long long sides = 6;
        if (!arguments_.empty()) {
            try {
                sides = std::stoll(arguments_[0]);
            } catch (const std::exception&) {
                std::cerr << "Usage: dice [sides]\n";
                return 1;
            }
        }

        if (sides <= 0) {
            std::cerr << "dice: sides must be greater than zero\n";
            return 1;
        }

        static std::random_device rd;
        static std::mt19937_64 generator(rd());
        std::uniform_int_distribution<long long> dist(1, sides);
        std::cout << dist(generator) << '\n';
        return 0;
    }

    if (name_ == "coinflip") {
        static std::random_device rd;
        static std::mt19937_64 generator(rd());
        std::uniform_int_distribution<int> dist(0, 1);
        std::cout << (dist(generator) == 0 ? "Heads" : "Tails") << '\n';
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

    if (name_ == "uud") {
        if (arguments_.empty()) {
            std::cerr << "Usage: uud [-e|-d] <text>\n";
            return 1;
        }

        try {
            const std::string mode = arguments_[0];
            const std::string value = arguments_.size() > 1 ? joinArguments(std::vector<std::string>(arguments_.begin() + 1, arguments_.end())) : "";

            if (mode == "-d" || mode == "--decode") {
                std::cout << uuDecode(value) << '\n';
                return 0;
            }

            if (mode == "-e" || mode == "--encode") {
                std::cout << uuEncode(value) << '\n';
                return 0;
            }

            std::cout << uuEncode(joinArguments(arguments_)) << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "uud: " << ex.what() << '\n';
            return 1;
        }
    }

    if (name_ == "weather") {
        std::filesystem::path projectRoot = std::filesystem::current_path();
        std::filesystem::path scriptPath = projectRoot / "scripts" / "weather.py";

        if (!std::filesystem::exists(scriptPath)) {
            std::filesystem::path altPath = projectRoot.parent_path() / "scripts" / "weather.py";
            if (std::filesystem::exists(altPath)) {
                scriptPath = altPath;
            }
        }

        if (!std::filesystem::exists(scriptPath)) {
            std::cerr << "weather: script not found at " << scriptPath << "\n";
            return 1;
        }

#ifdef _WIN32
        std::filesystem::path pythonPath = std::filesystem::path("C:/Users/Tayo Fatox/AppData/Local/Programs/Python/Python313/python.exe");
        pythonPath = pythonPath.make_preferred();
        std::wstring python = toWide(pythonPath.string());
        std::wstring script = toWide(scriptPath.make_preferred().string());
        std::wstring commandLine = L"\"" + python + L"\" \"" + script + L"\"";
        for (const auto& arg : arguments_) {
            commandLine += L" \"" + toWide(arg) + L"\"";
        }

        std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
        mutableCommand.push_back(L'\0');

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};

        const BOOL created = CreateProcessW(
            python.c_str(),
            mutableCommand.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

        if (!created) {
            const DWORD error = GetLastError();
            std::cerr << "weather: unable to launch Python interpreter (error " << error << ")\n";
            return 1;
        }

        WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(processInfo.hProcess, &exitCode);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return static_cast<int>(exitCode);
#else
        std::string pythonExe = "/usr/bin/python3";
        std::string command = "\"" + pythonExe + "\" \"" + scriptPath.string() + "\"";
        for (const auto& arg : arguments_) {
            command += " \"" + arg + "\"";
        }

        const int result = std::system(command.c_str());
        return result;
#endif
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

        const std::vector<std::string> resolved = resolveCommandChainArguments(arguments_, name_);
        const std::string value = joinArguments(resolved);
        std::cout << base64Encode(value) << '\n';
        return 0;
    }

    if (name_ == "b64decode") {
        if (arguments_.empty()) {
            std::cerr << "Usage: b64decode <text>\n";
            return 1;
        }

        try {
            const std::vector<std::string> resolved = resolveCommandChainArguments(arguments_, name_);
            const std::string value = joinArguments(resolved);
            const std::string decoded = base64Decode(value);
            std::cout << decoded << '\n';
            return 0;
        } catch (const std::exception& ex) {
            std::cerr << "b64decode: " << ex.what() << '\n';
            return 1;
        }
    }

    if (name_ == "specs") {
        std::string cpu = "Unknown";
        std::string gpu = "Unknown";
        std::string ram = "Unknown";
        std::string storage = "Unknown";
        std::string motherboard = "Unknown";

#ifdef _WIN32
        cpu = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"(Get-CimInstance Win32_Processor).Name\" 2>nul"));
        if (cpu == "Unknown") {
            cpu = firstMeaningfulLine(commandOutput("wmic cpu get Name 2>nul | findstr /V \"Name\""));
        }

        gpu = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"(Get-CimInstance Win32_VideoController).Name\" 2>nul"));
        if (gpu == "Unknown") {
            gpu = firstMeaningfulLine(commandOutput("wmic path win32_VideoController get Name 2>nul | findstr /V \"Name\""));
        }

        const std::string ramRaw = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"(Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory\" 2>nul"));
        try {
            if (ramRaw != "Unknown") {
                const std::uint64_t bytes = std::stoull(ramRaw);
                ram = formatBytesToGiB(bytes);
            }
        } catch (const std::exception&) {
            ram = "Unknown";
        }

        storage = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"Get-CimInstance Win32_DiskDrive | Select-Object -First 1 @{Name='DiskInfo';Expression={ $_.Model + ' (' + [math]::Round($_.Size / 1GB, 2) + ' GiB)' }} | Select-Object -ExpandProperty DiskInfo\" 2>nul"));
        if (storage == "Unknown") {
            storage = firstMeaningfulLine(commandOutput("wmic DiskDrive get Model,Size 2>nul | findstr /V \"Model Size\""));
        }
        if (storage == "Unknown" || storage == "Windows") {
            storage = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"Get-CimInstance Win32_LogicalDisk | Where-Object { $_.DriveType -eq 3 } | Select-Object -First 1 @{Name='DriveInfo';Expression={ $_.DeviceID + ' ' + [math]::Round($_.Size / 1GB, 2) + ' GiB' }} | Select-Object -ExpandProperty DriveInfo\" 2>nul"));
        }

        motherboard = firstMeaningfulLine(commandOutput("powershell -NoProfile -Command \"(Get-CimInstance Win32_BaseBoard | Select-Object Manufacturer,Product | Format-Table -HideTableHeaders | Out-String).Trim()\" 2>nul"));
        if (motherboard == "Unknown") {
            motherboard = firstMeaningfulLine(commandOutput("wmic baseboard get Product,Manufacturer 2>nul | findstr /V \"Product Manufacturer\""));
        }
#else
#ifdef __APPLE__
        cpu = firstMeaningfulLine(commandOutput("sysctl -n machdep.cpu.brand_string 2>/dev/null"));
        gpu = firstMeaningfulLine(commandOutput("system_profiler SPDisplaysDataType 2>/dev/null | grep 'Chipset' | head -n 1"));
        if (gpu == "Unknown") {
            gpu = firstMeaningfulLine(commandOutput("system_profiler SPDisplaysDataType 2>/dev/null | grep 'Displays:' -A 20 | tail -n +2 | head -n 1"));
        }

        const std::string memRaw = firstMeaningfulLine(commandOutput("sysctl -n hw.memsize 2>/dev/null"));
        try {
            if (memRaw != "Unknown") {
                ram = formatBytesToGiB(std::stoull(memRaw));
            }
        } catch (const std::exception&) {
            ram = "Unknown";
        }

        storage = firstMeaningfulLine(commandOutput("system_profiler SPStorageDataType 2>/dev/null | grep 'Capacity' | head -n 1"));
        if (storage == "Unknown") {
            storage = firstMeaningfulLine(commandOutput("df -h / 2>/dev/null | tail -n +2 | head -n 1"));
        }

        motherboard = firstMeaningfulLine(commandOutput("sysctl -n hw.model 2>/dev/null"));
#else
        std::ifstream cpuInfo("/proc/cpuinfo");
        if (cpuInfo) {
            std::string line;
            while (std::getline(cpuInfo, line)) {
                if (line.rfind("model name", 0) == 0 || line.rfind("Hardware", 0) == 0 || line.rfind("Processor", 0) == 0) {
                    const std::size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        cpu = normalizeWhitespace(line.substr(colon + 1));
                        break;
                    }
                }
            }
        }

        gpu = firstMeaningfulLine(commandOutput("lspci 2>/dev/null | grep -iE 'vga|3d|display' | head -n 1"));
        if (gpu == "Unknown") {
            gpu = firstMeaningfulLine(commandOutput("glxinfo -B 2>/dev/null | grep -i 'OpenGL renderer string' | head -n 1"));
        }

        std::ifstream memInfo("/proc/meminfo");
        if (memInfo) {
            std::string line;
            while (std::getline(memInfo, line)) {
                if (line.rfind("MemTotal:", 0) == 0) {
                    const std::size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        const std::string value = trim(line.substr(colon + 1));
                        const std::size_t space = value.find(' ');
                        if (space != std::string::npos) {
                            try {
                                const std::uint64_t kb = std::stoull(value.substr(0, space));
                                ram = formatBytesToGiB(kb * 1024ULL);
                            } catch (const std::exception&) {
                                ram = "Unknown";
                            }
                        }
                        break;
                    }
                }
            }
        }

        storage = firstMeaningfulLine(commandOutput("lsblk -dn -o NAME,SIZE 2>/dev/null | head -n 1"));
        if (storage == "Unknown") {
            storage = firstMeaningfulLine(commandOutput("df -h / 2>/dev/null | tail -n +2 | head -n 1"));
        }

        std::ifstream vendorFile("/sys/class/dmi/id/board_vendor");
        std::ifstream productFile("/sys/class/dmi/id/board_name");
        std::string vendor, product;
        if (vendorFile) { std::getline(vendorFile, vendor); }
        if (productFile) { std::getline(productFile, product); }
        vendor = trim(vendor);
        product = trim(product);
        if (!vendor.empty() || !product.empty()) {
            motherboard = vendor + (vendor.empty() || product.empty() ? "" : " ") + product;
        }
#endif
#endif

        const std::string accent = themeColorCode(currentTheme());
        std::cout << accent << "System Specs\n" << "\033[0m";
        std::cout << accent << "CPU: " << "\033[0m" << cpu << '\n';
        std::cout << accent << "GPU: " << "\033[0m" << gpu << '\n';
        std::cout << accent << "RAM: " << "\033[0m" << ram << '\n';
        std::cout << accent << "Storage: " << "\033[0m" << storage << '\n';
        std::cout << accent << "Motherboard: " << "\033[0m" << motherboard << '\n';
        return 0;
    }

    if (name_ == "req") {
        if (arguments_.empty()) {
            std::cerr << "Usage: req <url> [-X METHOD] [-H \"Header: value\"] [-d DATA]\n";
            return 1;
        }

        std::string method = "GET";
        std::string data;
        std::string url;
        std::vector<std::string> headers;

        for (std::size_t i = 0; i < arguments_.size(); ++i) {
            const std::string& arg = arguments_[i];
            if (arg == "-X" || arg == "--request") {
                if (i + 1 >= arguments_.size()) {
                    std::cerr << "req: missing request method\n";
                    return 1;
                }
                method = arguments_[++i];
            } else if (arg == "-H" || arg == "--header") {
                if (i + 1 >= arguments_.size()) {
                    std::cerr << "req: missing header value\n";
                    return 1;
                }
                headers.push_back(arguments_[++i]);
            } else if (arg == "-d" || arg == "--data" || arg == "--data-raw") {
                if (i + 1 >= arguments_.size()) {
                    std::cerr << "req: missing request data\n";
                    return 1;
                }
                data = arguments_[++i];
            } else if (arg == "-L" || arg == "--location") {
                continue;
            } else if (url.empty()) {
                url = arg;
            } else {
                data = joinArguments(std::vector<std::string>(arguments_.begin() + static_cast<std::ptrdiff_t>(i), arguments_.end()));
                break;
            }
        }

        if (url.empty()) {
            std::cerr << "Usage: req <url> [-X METHOD] [-H \"Header: value\"] [-d DATA]\n";
            return 1;
        }

        std::string command = "curl -sS -L";
        if (method != "GET") {
            command += " -X " + escapeShellArgument(method);
        }
        for (const auto& header : headers) {
            command += " -H " + escapeShellArgument(header);
        }
        if (!data.empty()) {
            command += " --data " + escapeShellArgument(data);
        }
        command += " " + escapeShellArgument(url);

        const int curlStatus = std::system(command.c_str());
        if (curlStatus == 0) {
            return 0;
        }

#ifdef _WIN32
        const std::string pythonExe = "python";
#else
        const std::string pythonExe = "/usr/bin/env python3";
#endif

        std::string pythonCommand = "";
        pythonCommand += pythonExe + " -c \"import json, sys, urllib.request, urllib.error; "
            "url = sys.argv[1]; method = sys.argv[2] if len(sys.argv) > 2 else 'GET'; data = sys.argv[3] if len(sys.argv) > 3 else None; "
            "headers = {}; "
            "for item in sys.argv[4:]: headers[item.split(':', 1)[0].strip()] = item.split(':', 1)[1].strip() if ':' in item else ''; "
            "req = urllib.request.Request(url, data=(data.encode() if isinstance(data, str) else None), headers=headers, method=method); "
            "try: response = urllib.request.urlopen(req, timeout=15); body = response.read().decode('utf-8', 'replace'); print(body, end=''); "
            "except Exception as exc: print(str(exc), file=sys.stderr); raise SystemExit(1)\" " + escapeShellArgument(url) + " " + escapeShellArgument(method);

        if (!data.empty()) {
            pythonCommand += " " + escapeShellArgument(data);
        }
        for (const auto& header : headers) {
            pythonCommand += " " + escapeShellArgument(header);
        }

        return std::system(pythonCommand.c_str());
    }

    if (name_ == "gridview" || name_ == "grid" || name_ == "gv") {
        if (arguments_.empty()) {
            std::cerr << "Usage: gridview [--no-color] [--limit N] [--sep ,|;|tab] <csv|tsv|file|command>\n";
            return 1;
        }

        bool colorEnabled = true;
        std::size_t limit = 0;
        char delimiter = ',';
        std::string source;

        for (std::size_t i = 0; i < arguments_.size(); ++i) {
            const std::string& arg = arguments_[i];
            if (arg == "--no-color") {
                colorEnabled = false;
            } else if (arg == "--limit" || arg == "-l") {
                if (i + 1 >= arguments_.size()) {
                    std::cerr << "gridview: missing limit value\n";
                    return 1;
                }
                try {
                    limit = static_cast<std::size_t>(std::stoul(arguments_[++i]));
                } catch (const std::exception&) {
                    std::cerr << "gridview: invalid limit value\n";
                    return 1;
                }
            } else if (arg == "--sep" || arg == "-s") {
                if (i + 1 >= arguments_.size()) {
                    std::cerr << "gridview: missing separator value\n";
                    return 1;
                }
                const std::string sepArg = arguments_[++i];
                if (sepArg == "tab" || sepArg == "\t") {
                    delimiter = '\t';
                } else if (sepArg == ";") {
                    delimiter = ';';
                } else if (sepArg == "|") {
                    delimiter = '|';
                } else if (sepArg == "," || sepArg == "csv") {
                    delimiter = ',';
                } else {
                    delimiter = sepArg[0];
                }
            } else if (source.empty()) {
                source = arg;
            } else {
                source += " " + arg;
            }
        }

        if (source.empty()) {
            std::cerr << "Usage: gridview [--no-color] [--limit N] [--sep ,|;|tab] <csv|tsv|file|command>\n";
            return 1;
        }

        std::string content;
        const std::filesystem::path candidate = std::filesystem::path(source);
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            std::ifstream input(candidate, std::ios::binary);
            if (!input) {
                std::cerr << "gridview: unable to open file: " << source << '\n';
                return 1;
            }
            std::ostringstream buffer;
            buffer << input.rdbuf();
            content = stripAnsiCodes(buffer.str());
        } else {
            const std::string maybeCommand = trim(source);
            const bool looksLikeData = maybeCommand.find(',') != std::string::npos || maybeCommand.find('\t') != std::string::npos || maybeCommand.find(';') != std::string::npos || maybeCommand.find('\n') != std::string::npos;
            if (!looksLikeData) {
                content = stripAnsiCodes(commandOutput(maybeCommand));
            } else {
                content = maybeCommand;
            }
        }

        if (trim(content).empty()) {
            std::cout << "(no data)\n";
            return 0;
        }

        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::filesystem::path tempCsv = tempDir / ("pelagia_gridview_" + std::to_string(std::time(nullptr)) + ".csv");
        std::ofstream tempFile(tempCsv, std::ios::binary);
        if (!tempFile) {
            std::cerr << "gridview: unable to create temporary data file\n";
            return 1;
        }
        tempFile << content;
        tempFile.close();

#ifdef _WIN32
        std::vector<std::filesystem::path> pythonCandidates = {
            std::filesystem::path("C:/Users/Tayo Fatox/AppData/Local/Programs/Python/Python313/pythonw.exe"),
            std::filesystem::path("C:/Users/Tayo Fatox/AppData/Local/Programs/Python/Python313/python.exe"),
            std::filesystem::path("C:/Windows/System32/pythonw.exe"),
            std::filesystem::path("C:/Windows/System32/python.exe")
        };

        std::filesystem::path pythonPath;
        for (const auto& candidate : pythonCandidates) {
            if (std::filesystem::exists(candidate)) {
                pythonPath = candidate;
                break;
            }
        }

        if (pythonPath.empty()) {
            pythonPath = std::filesystem::path("python");
        }

        const std::filesystem::path projectRoot = std::filesystem::current_path();
        std::filesystem::path scriptPath = projectRoot / "scripts" / "gridview_gui.py";
        if (!std::filesystem::exists(scriptPath)) {
            const std::filesystem::path altScript = projectRoot.parent_path() / "scripts" / "gridview_gui.py";
            if (std::filesystem::exists(altScript)) {
                scriptPath = altScript;
            }
        }

        if (!std::filesystem::exists(scriptPath)) {
            std::cerr << "gridview: script not found at " << scriptPath << '\n';
            std::filesystem::remove(tempCsv);
            return 1;
        }

        const std::string delimiterArg = delimiter == '\t' ? "tab" : std::string(1, delimiter);
        const std::string themeArg = currentTheme();
        const std::wstring pythonWide = toWide(pythonPath.make_preferred().string());
        const std::wstring scriptWide = toWide(scriptPath.make_preferred().string());
        const std::wstring csvWide = toWide(tempCsv.make_preferred().string());
        const std::wstring delimiterWide = toWide(delimiterArg);
        const std::wstring themeWide = toWide(themeArg);
        const std::wstring titleWide = toWide("PelagiaShell GridView");

        std::wstring commandLine = L"\"" + pythonWide + L"\" \"" + scriptWide + L"\" --csv \"" + csvWide + L"\" --delimiter \"" + delimiterWide + L"\" --title \"" + titleWide + L"\" --theme \"" + themeWide + L"\"";
        std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
        mutableCommand.push_back(L'\0');

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        const BOOL created = CreateProcessW(
            nullptr,
            mutableCommand.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_DEFAULT_ERROR_MODE,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo
        );

        if (!created) {
            const DWORD error = GetLastError();
            std::cerr << "gridview: unable to launch GUI window (CreateProcess failed, error " << error << ")\n";
            return 1;
        }

        CloseHandle(processInfo.hThread);
        std::thread([processHandle = processInfo.hProcess, csvPath = tempCsv]() {
            if (processHandle != nullptr) {
                WaitForSingleObject(processHandle, INFINITE);
                CloseHandle(processHandle);
            }
            std::filesystem::remove(csvPath);
        }).detach();

        return 0;
#else
        std::string pythonExe = "/usr/bin/env python3";

        const std::filesystem::path projectRoot = std::filesystem::current_path();
        std::filesystem::path scriptPath = projectRoot / "scripts" / "gridview_gui.py";
        if (!std::filesystem::exists(scriptPath)) {
            const std::filesystem::path altScript = projectRoot.parent_path() / "scripts" / "gridview_gui.py";
            if (std::filesystem::exists(altScript)) {
                scriptPath = altScript;
            }
        }

        const std::string delimiterArg = delimiter == '\t' ? "tab" : std::string(1, delimiter);
        const std::string themeArg = currentTheme();
        std::string command = "";
        command += pythonExe + " " + escapeShellArgument(scriptPath.string()) + " --csv " + escapeShellArgument(tempCsv.string()) + " --delimiter " + escapeShellArgument(delimiterArg) + " --title " + escapeShellArgument("PelagiaShell GridView") + " --theme " + escapeShellArgument(themeArg);

        const int scriptStatus = std::system(command.c_str());
        std::filesystem::remove(tempCsv);
        return scriptStatus;
#endif
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
        std::cout << "PelagiaShell builtins: echo, cd, pwd, ls, mkdir, rmdir, touch, cat, clear, date, whoami, uname, rand, random, dice, coinflip, uud, weather, specs, req, gridview, grid, gv, b64encode, b64decode, about, theme, motd, banner, help, exit\n";
        return 0;
    }

    return 1;
}
