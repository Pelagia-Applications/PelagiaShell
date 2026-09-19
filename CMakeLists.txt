cmake_minimum_required(VERSION 3.16)
project(PelagiaShell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PelagiaShell
    src/main.cpp
    src/Shell.cpp
    src/Parser.cpp
    src/Command.cpp
    src/Process.cpp
    src/Terminal.cpp
)

target_include_directories(PelagiaShell
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
