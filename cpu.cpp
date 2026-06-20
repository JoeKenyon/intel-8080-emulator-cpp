#include "cpu.h"
#include <iostream>

// https://k1.spdns.de/Develop/Hardware/Infomix/ICs%20computer/Microprocessor/8080%20By%20Opcode.html

CPU::CPU(Bus& bus) : bus(bus)
{
    PC = 0x0000;
    SP = 0x0000;

    regs.A = regs.B = regs.C = regs.D = regs.E = regs.H = regs.L = 0;
    flags.Zero = flags.Sign = flags.Parity = flags.Carry = flags.AuxCarry = 0;
}

void CPU::step()
{
    uint8_t opcode = bus.read8(PC);
    uint8_t op1    = bus.read8((uint16_t)(PC + 1));
    uint8_t op2    = bus.read8((uint16_t)(PC + 2));

    PC += OpcodeTable[opcode].size;

    execute(opcode, op1, op2);
}

// we can use function pointers
void CPU::execute(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    (this->*dispatch[opcode])(opcode, op1, op2);
}

/*
    The dispatch table, every opcode should have its own handler
    table[OPCODE_ENUM] = &CPU::opcodeFunction;
*/
std::array<CPU::OpHandler, OPCODE_NUM> CPU::buildDispatchTable()
{
    std::array<OpHandler, OPCODE_NUM> table{};
    table.fill(&CPU::op_unimplemented);

    table[NOP]    = &CPU::op_nop;
    table[MVI_A]  = &CPU::op_mvi_a;
    table[CPI]    = &CPU::op_cpi;
    table[LXI_B]  = &CPU::op_lxi_b;
    table[STAX_B] = &CPU::op_stax_b;

    return table;
}

const std::array<CPU::OpHandler, OPCODE_NUM> CPU::dispatch = CPU::buildDispatchTable();

// all our handlers
void CPU::op_unimplemented(uint8_t opcode, uint8_t /*op1*/, uint8_t /*op2*/)
{
    std::cerr << "Unimplemented opcode: 0x" << std::hex << static_cast<int>(opcode)
               << " (" << OpcodeTable[opcode].mnemonic << ") at PC=0x"
               << (PC - OpcodeTable[opcode].size) << std::dec << "\n";
}

void CPU::op_nop(uint8_t, uint8_t, uint8_t)
{
}

void CPU::op_mvi_a(uint8_t, uint8_t op1, uint8_t)
{
    regs.A = op1;
}

void CPU::op_cpi(uint8_t, uint8_t op1, uint8_t)
{
    // TODO: compare A against op1, set Zero/Sign/Parity/Carry/AuxCarry
    (void)op1;
}

void CPU::op_lxi_b(uint8_t, uint8_t op1, uint8_t op2)
{
    regs.C = op1;   // low byte
    regs.B = op2;   // high byte
}

void CPU::op_stax_b(uint8_t, uint8_t, uint8_t)
{
    uint16_t addr = (static_cast<uint16_t>(regs.B) << 8) | regs.C;
    bus.write8(addr, regs.A);
}