#pragma once

#include <string>

inline std::string& currentTheme() {
    static std::string theme = "ocean";
    return theme;
}

inline std::string& currentMotd() {
    static std::string motd = "A tiny shell with a custom tide.";
    return motd;
}

inline std::string themeLabel(const std::string& theme) {
    if (theme == "sunset") {
        return "sunset";
    }
    if (theme == "neon") {
        return "neon";
    }
    if (theme == "hacker") {
        return "hacker";
    }
    return "ocean";
}

inline std::string themeColorCode(const std::string& theme) {
    if (theme == "sunset") {
        return "\033[38;5;214m";
    }
    if (theme == "neon") {
        return "\033[38;5;201m";
    }
    if (theme == "hacker") {
        return "\033[38;5;46m";
    }
    return "\033[38;5;39m";
}

inline std::string promptColorCode(const std::string& theme) {
    if (theme == "sunset") {
        return "\033[38;5;208m";
    }
    if (theme == "neon") {
        return "\033[38;5;46m";
    }
    if (theme == "hacker") {
        return "\033[38;5;82m";
    }
    return "\033[38;5;117m";
}

inline std::string inputColorCode(const std::string& theme) {
    if (theme == "sunset") {
        return "\033[38;5;226m";
    }
    if (theme == "neon") {
        return "\033[38;5;213m";
    }
    if (theme == "hacker") {
        return "\033[38;5;10m";
    }
    return "\033[38;5;255m";
}

inline std::string themeTitle(const std::string& theme) {
    if (theme == "sunset") {
        return "PelagiaShell // sunset drift";
    }
    if (theme == "neon") {
        return "PelagiaShell // neon tide";
    }
    if (theme == "hacker") {
        return "PelagiaShell // hacker mode";
    }
    return "PelagiaShell // oceanic shell";
}
