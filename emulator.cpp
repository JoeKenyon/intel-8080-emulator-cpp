#include "hardware.h"
#include "opcodes.h"
#include <iostream>
#include <fstream>
#include <iomanip>


// load roms into memory using start address
bool loadRom(const std::string& filename, uint16_t startAddress)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (startAddress + fileSize > 65536)
    {
        std::cerr << "Error: ROM file is too large for memory.\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&memory[startAddress]), fileSize);

    return true;
}

// cpu step
bool step()
{
    // get opcode
    uint8_t opcode = memory[PC];

    // get operands
    uint8_t op1 = memory[PC + 1];
    uint8_t op2 = memory[PC + 2];

    std::cout << "[ "
              << std::left << std::setw(10) << std::setfill(' ') << DISSAMBLER_STATES[opcode]
              << " ] 0x"
              << std::hex << std::setw(2) << std::setfill('0') << (int)opcode
              << "\n";
    switch (opcode)
    {
        case OP_NOP: // NOP (No Operation)
        {
            PC += OPCODE_CYCLES[opcode]; // It's a 1-byte instruction, just move past it
            break;
        }

        case OP_JMP: // Unconditional JMP
        {
            // Intel 8080 is little-endian: low-byte (op1) then high-byte (op2)
            uint16_t targetAddress = (op2 << 8) | op1;

            PC = targetAddress; // move the PC to point to where to jump to
            break;
        }

        case OP_LXI_SP: //LXI SP,d16
        {
            uint16_t immediateValue = (op2 << 8) | op1;
            SP = immediateValue;
            PC += OPCODE_CYCLES[opcode];
            break;
        }

        case OP_ANI: //ANI d8 bit wise and immediant value
        {
            regs.A = regs.A & op1; // use op1

            // update flags based on register value
            flags.Zero     = __calculateZero(regs.A);
            flags.Sign     = __calculateSign(regs.A);
            flags.Carry    = 0;
            flags.Parity   = __calculateParity(regs.A);
            flags.AuxCarry = __calculateAuxCarryANI(op1);

            PC += OPCODE_CYCLES[opcode];
            break;
        }

        default:
            std::cerr << "Unimplemented opcode: 0x"<< std::hex << (int)opcode << " at PC=0x" << PC << "\n";
            return false;
    }

    return true;
}
