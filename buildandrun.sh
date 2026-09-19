cmake -S . -B build
cmake --build build

read -p "Do you want to run the program? (y/n): " answer
if [[ $answer == "y" || $answer == "Y" ]]; then
    ./build/PelagiaShell.exe
else
    echo "Build completed."
    echo Closing...
    wait 2
fi