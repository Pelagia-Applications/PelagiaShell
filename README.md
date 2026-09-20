# PelagiaShell

PelagiaShell is a lightweight, custom shell built in C++ with a playful oceanic theme and a few built-in utilities designed to feel more like a tiny personal command environment than a basic terminal wrapper.

## Features

- Built-in command set for common shell tasks
- Theme switching with multiple looks
- Custom message of the day and banner text
- Simple interactive shell loop
- Cross-platform command behavior where practical

## Included commands

- echo
- cd
- pwd
- ls
- mkdir
- rmdir
- touch
- cat
- clear
- date
- whoami
- uname
- rand
- random
- b64encode
- b64decode
- about
- theme
- motd
- banner
- help
- exit

## Available themes

- ocean
- sunset
- neon
- hacker

## Quick start

From the project root, build with CMake and run the executable:

```bash
cmake -S . -B build
cmake --build build
./build/PelagiaShell
```

On Windows, use the generated executable in the build directory, for example:

```powershell
cmake -S . -B build
cmake --build build
.\build\PelagiaShell.exe
```

## Example usage

```text
pelagia~> help
pelagia~> theme sunset
pelagia~> motd welcome to my shell
pelagia~> rand 1 100
pelagia~> b64encode hello
```

## Notes

PelagiaShell is intentionally small and scriptable. It is designed as a personal shell with themed presentation and a compact built-in command set rather than a full Unix shell replacement.
