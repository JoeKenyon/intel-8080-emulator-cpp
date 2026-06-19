#include "cpu.h"


// https://k1.spdns.de/Develop/Hardware/Infomix/ICs%20computer/Microprocessor/8080%20By%20Opcode.html
// http://www.eazynotes.com/notes/microprocessor/notes/opcodes-table-of-intel-8085.pdf

CPU::CPU()
{
    rom  = &memory[0x0000]; // ROM starts at 0x0000 - 0x1FFF
    ram  = &memory[0x2000]; // Work RAM starts at 0x2000 - 0x23FF
    vram = &memory[0x2400]; // Video RAM starts at 0x2400 - 0x3FFF

    // zero out the memory array
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

bool CPU::loadRom(const std::string& filename, uint16_t startAddress)
{
    // load the ifle in binary mode
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return false;
    }

    // get filesize to validate it
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (startAddress + fileSize > MEMORY_SIZE)
     {
        std::cerr << "Error: ROM file is too large for the remaining memory space.\n";
        return false;
    }

    // read into rom at specified address... for using testing roms
    if (!file.read(reinterpret_cast<char*>(&memory[startAddress]), fileSize)) {
        std::cerr << "Error: Failed to read file data.\n";
        return false;
    }

    return true;
}

void CPU::execute(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    switch (opcode) 
    {
        case NOP:
            break;

        case LXI_B:
            regs.B = op1;
            break;

        case STAX_B:
            regs.A = op1;
            break;

        case 0x01:
            regs.C = op1;
            regs.B = op2;
            break;
        
        case 0xC3:
            PC = (op2 << 8) | op1;
    }
}