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
              << std::left << std::setw(10) << std::setfill(' ') << MNEMONICS[opcode]
              << " ] 0x"
              << std::hex << std::setw(2) << std::setfill('0') << (int)opcode
              << "\n";

    switch (opcode)
    {
        case OP_NOP: PC += 1; break;
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

        case OP_ADI: op_ADI(op1); break;

        // add
        case OP_ADD_A: op_ADD(regs.A); break;
        case OP_ADD_B: op_ADD(regs.B); break;
        case OP_ADD_C: op_ADD(regs.C); break;
        case OP_ADD_D: op_ADD(regs.D); break;
        case OP_ADD_E: op_ADD(regs.E); break;
        case OP_ADD_H: op_ADD(regs.H); break;
        case OP_ADD_L: op_ADD(regs.L); break;
        case OP_ADD_M: op_ADD(memory[(regs.H << 8) | regs.L]); break;

        case OP_SUB_A: op_SUB(regs.A); break;
        case OP_SUB_B: op_SUB(regs.B); break;
        case OP_SUB_C: op_SUB(regs.C); break;
        case OP_SUB_D: op_SUB(regs.D); break;
        case OP_SUB_E: op_SUB(regs.E); break;
        case OP_SUB_H: op_SUB(regs.H); break;
        case OP_SUB_L: op_SUB(regs.L); break;
        case OP_SUB_M: op_SUB(memory[(regs.H << 8) | regs.L]); break;

        case OP_SUI: op_SUI(op1); break;

        case OP_INR_A: op_INR(regs.A); break;
        case OP_INR_B: op_INR(regs.B); break;
        case OP_INR_C: op_INR(regs.C); break;
        case OP_INR_D: op_INR(regs.D); break;
        case OP_INR_E: op_INR(regs.E); break;
        case OP_INR_H: op_INR(regs.H); break;
        case OP_INR_L: op_INR(regs.L); break;
        case OP_INR_M: op_INR(memory[(regs.H << 8) | regs.L]); break;

        case OP_DCR_A: op_DCR(regs.A); break;
        case OP_DCR_B: op_DCR(regs.B); break;
        case OP_DCR_C: op_DCR(regs.C); break;
        case OP_DCR_D: op_DCR(regs.D); break;
        case OP_DCR_E: op_DCR(regs.E); break;
        case OP_DCR_H: op_DCR(regs.H); break;
        case OP_DCR_L: op_DCR(regs.L); break;
        case OP_DCR_M: op_DCR(memory[(regs.H << 8) | regs.L]); break;

        default: std::cerr << "Unimplemented opcode: 0x"<< std::hex << (int)opcode << " at PC=0x" << PC << "\n"; return false;
    }

    return true;
}
