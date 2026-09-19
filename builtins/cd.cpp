#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: cd <directory>\n";
        return 1;
    }

    std::cout << "cd to " << argv[1] << "\n";
    return 0;
}
