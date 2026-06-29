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
        case OP_NOP: PC += OPCODE_CYCLES[opcode]; break;
        case OP_LXI_SP: op_LXI_SP(op1, op2); break;
        case OP_ANI: op_ANI(op1); break;

        // jumping
        case OP_JMP: op_JMP(op1, op2); break;
        case OP_JC : op_JC (op1, op2); break;
        case OP_JNC: op_JNC(op1, op2); break;
        case OP_JZ : op_JZ (op1, op2); break;
        case OP_JNZ: op_JNZ(op1, op2); break;
        case OP_JPE: op_JPE(op1, op2); break;
        case OP_JPO: op_JPO(op1, op2); break;
        case OP_JM : op_JM (op1, op2); break;
        case OP_JP : op_JP (op1, op2); break;

        default: std::cerr << "Unimplemented opcode: 0x"<< std::hex << (int)opcode << " at PC=0x" << PC << "\n"; return false;
    }

    return true;
}
