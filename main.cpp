#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>

/*
print rom 
xxd roms/tetris.gb | head -n 25
*/

int main() 
{
    // open the ROM in binary mode
    std::ifstream file("./roms/tetris.gb", std::ios::binary);
    
    if (!file.is_open()) 
    {
        std::cerr << "Error: Could not open the ROM file.\n";
        return 1;
    }
    char byte;

    file.read(&byte, sizeof(char));

    std::printf("0x%02X\n", static_cast<uint8_t>(byte));

    file.close();
    return 0;
}