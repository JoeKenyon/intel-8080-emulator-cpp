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

    cpu.PC = 0x0100;

    for(int i = 0; i < 10; i++)
    {
        uint8_t opcode = cpu.memory[cpu.PC];
        uint8_t operand_1 = cpu.memory[cpu.PC + 1];
        uint8_t operand_2 = cpu.memory[cpu.PC + 2];

        cpu.PC += cpu.OP_TABLE[opcode].size;

        cpu.execute(opcode, operand_1, operand_2);

        std::cout << cpu.OP_TABLE[opcode].mnemonic << std::endl;
    }




    return 0;
}