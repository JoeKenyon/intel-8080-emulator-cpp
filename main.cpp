#include <iostream>
#include "emulator.h"

int main()
{
    // load test rom
    if (!loadRom("roms/cpudiag.bin", 0x0100)) {
        return 1;
    }

    PC = 0x0100;
    SP = 0xF000;

    while(step());

    std::cout << "\nStep complete. Next PC target is: 0x" << std::hex << PC << "\n";
    std::cout << "Next opcode waiting to be implemented is: 0x" << (int)memory[PC] << "\n";

    return 0;
}
