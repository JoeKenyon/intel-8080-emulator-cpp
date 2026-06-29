#include <iostream>
#include "emulator.h"

int main()
{
    // Load the binary directly at CP/M's standard base address
    if (!loadRom("roms/cpudiag.bin", 0x0100)) {
        return 1;
    }

    // Set initial machine state pointers
    PC = 0x0100;
    SP = 0xF000;

    // Run the first step!
    while(step());

    std::cout << "\nStep complete. Next PC target is: 0x" << std::hex << PC << "\n";
    std::cout << "Next opcode waiting to be implemented is: 0x" << (int)memory[PC] << "\n";

    return 0;
}
