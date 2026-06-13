#include <iostream>
#include <fstream>
#include <iomanip>

int main() 
{
    // open the ROM in binary mode
    std::ifstream file("roms/tetris.gb", std::ios::binary);
    
    if (!file.is_open()) 
    {
        std::cerr << "Error: Could not open the ROM file.\n";
        return 1;
    }
    char byte;

    file.read(&byte, sizeof(char));

    std::cout << byte << "\n";

    file.close();
    return 0;
}