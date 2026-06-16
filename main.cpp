#include "cpu.h"
#include <iostream>

int main() 
{
    CPU cpu;

    // 1. Load the CP/M test binary at address 0x0100
    if (!cpu.loadRom("roms/8080PRE.COM", 0x0100)) 
    {
        return 1;
    }

    return 0;
}