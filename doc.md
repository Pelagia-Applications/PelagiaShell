# PelagiaShell Documentation

## Overview

PelagiaShell is a compact custom shell with a themed UI, a small built-in command set, and a few playful utility commands designed to feel more personal and expressive than a standard terminal prompt. It is implemented in C++ and centered around a shell loop, built-in execution, and visual theme customization.

## Shell model

PelagiaShell follows a simple command lifecycle:

1. Read a line from the terminal.
2. Parse it into tokens.
3. Check whether the command matches a built-in.
4. Run the matching built-in directly.
5. Otherwise fall back to the host OS command environment.

This makes the shell lightweight but still useful for everyday small tasks.

## Features

PelagiaShell supports the following feature set:

- Shell navigation and file operations
- Text and file output utilities
- Temperature of the shell identity: theme, MOTD, and banners
- Random and game-like utilities
- Encoding and decoding helpers
- Weather lookup support
- System specs reporting
- Operating-system-aware built-in behavior

## Built-in commands

### echo
Prints the supplied arguments to the terminal.

Example:

```text
pelagia~> echo hello world
hello world
```

### cd
Changes the current working directory.

Example:

```text
pelagia~> cd ../projects
```

### pwd
Prints the current working directory.

### ls
Lists the contents of the current directory.

### mkdir
Creates directories.

### rmdir
Removes directories.

### touch
Creates files if they do not already exist.

### cat
Displays the contents of a file.

### clear
Clears the terminal display.

### date
Displays the current date and time.

### whoami
Displays the current user name.

### uname
Displays a platform label such as the OS.

### rand
Generates a random integer within a specified range.

Example:

```text
pelagia~> rand 1 10
```

### random
Alias for rand.

### dice
Rolls a die using a configurable number of sides.

Example:

```text
pelagia~> dice 20
```

### coinflip
Returns a random result such as Heads or Tails.

### uud
Encodes or decodes a string using a simple UUD-style transformation.

Examples:

```text
pelagia~> uud -e hello
pelagia~> uud -d #:&5L";&\`
```

### weather
Runs a weather lookup helper, using the configured environment and local helper script when present.

This command is intended for external weather data access via an API-backed script.

### specs
Displays the system specification summary, including CPU, GPU, RAM, storage, and motherboard information when available.

### b64encode
Encodes a string as Base64.

Example:

```text
pelagia~> b64encode hello
```

### b64decode
Decodes a Base64 string.

Example:

```text
pelagia~> b64decode aGVsbG8=
```

### about
Displays the current shell theme and flavor message.

### theme
Shows the active theme or switches to a supported theme.

Supported themes:
- ocean
- sunset
- neon
- hacker

Example:

```text
pelagia~> theme sunset
```

### motd
Displays or updates the message of the day.

Example:

```text
pelagia~> motd welcome to PelagiaShell
```

### banner
Displays or updates the shell banner text.

### help
Shows the list of available built-ins.

### exit
Exits the shell.

## Themes

PelagiaShell ships with four built-in themes:

### ocean
Default shell palette.

### sunset
Warm orange and gold-inspired tones.

### neon
Bright neon-style colors and vibrant styling.

### hacker
Green monochrome terminal-inspired aesthetic.

## Customization

PelagiaShell supports shell identity customization through:

- theme <name>
- motd <message>
- banner <message>

These commands let the user personalize the shell branding without changing the underlying command set.

## Current command list

The shell currently includes the following commands:

```text
echo, cd, pwd, ls, mkdir, rmdir, touch, cat, clear, date, whoami, uname, rand, random, dice, coinflip, uud, weather, specs, b64encode, b64decode, about, theme, motd, banner, help, exit
```

## Implementation notes

PelagiaShell is intentionally compact and playful rather than a full replacement for a large Unix shell. The goal is to provide a user-friendly personal shell with a distinctive look, a small curated command set, and a few niche utilities that make it feel more custom and expressive.

## Supported platforms

PelagiaShell is designed to be portable and is built with CMake and C++17. The shell uses platform-aware logic where needed so it can behave consistently on Windows and Unix-like systems.
