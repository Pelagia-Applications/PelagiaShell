#include <iostream>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::cout << argv[i] << (i + 1 < argc ? " " : "");
    }
    std::cout << '\n';
    return 0;
}
