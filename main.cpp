#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>
#include "cpu.h"



int main() 
{
    CPU cpu;

    // load the rom

    cpu.loadRom("roms/space-invaders.bin");

    uint8_t opcode = cpu.memory[cpu.PC];
    cpu.PC += cpu.OP_TABLE[opcode].size;




    return 0;
}