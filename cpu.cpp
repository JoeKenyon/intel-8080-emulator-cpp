#include "cpu.h"

CPU::CPU()
{
    rom  = &memory[0x0000]; // ROM starts at 0x0000 - 0x1FFF
    ram  = &memory[0x2000]; // Work RAM starts at 0x2000 - 0x23FF
    vram = &memory[0x2400]; // Video RAM starts at 0x2400 - 0x3FFF

    // zero out the memory array🧹
    for (size_t i = 0; i < MEMORY_SIZE; ++i)
    {
        memory[i] = 0;
    }

    // set program counter and stack pointer
    PC = 0x0000; 
    SP = 0x0000;

    // clear registers and flags
    regs.A = regs.B = regs.C = regs.D = regs.E = regs.H = regs.L = 0;
    flags.Zero = flags.Sign = flags.Parity = flags.Carry = flags.AuxCarry = 0;
}
CPU::~CPU()
{

}