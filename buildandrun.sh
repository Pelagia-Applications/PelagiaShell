# !bin/bash

cmake -S . -B build
cmake --build build

if [ $? -ne 0 ]; then
    echo "Build failed. Exiting..."
    exit 1
fi
read -p "Would you like to run the program? (y/n): " choice
if [[ "$choice" == "y" || "$choice" == "Y" ]]; then
    ./build/PelagiaShell
else
    echo "Exiting..."
fi