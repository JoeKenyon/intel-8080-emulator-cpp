#pragma once
#include <cstdint>
#include <array>
#include "bus.h"
#include "opcodes.h"

class CPU
{
public:
    explicit CPU(Bus& bus);

    void step();    // fetch, decode, execute one instruction, advance PC
    void execute(uint8_t opcode, uint8_t op1, uint8_t op2);

    struct Registers
    {
        uint8_t A, B, C, D, E, H, L;
    };

    struct Flags
    {
        uint8_t Zero     : 1;
        uint8_t Sign     : 1;
        uint8_t Parity   : 1;
        uint8_t Carry    : 1;
        uint8_t AuxCarry : 1;
    };

    uint16_t PC;
    uint16_t SP;
    Flags flags;
    Registers regs;

private:
    Bus& bus;

    // every opcode function gets the same signature
    using OpHandler = void (CPU::*)(uint8_t opcode, uint8_t op1, uint8_t op2);

    static std::array<OpHandler, OPCODE_NUM> buildDispatchTable();
    static const std::array<OpHandler, OPCODE_NUM> dispatch;

    // if we havent implemented it yet, maybe delete later
    void op_unimplemented(uint8_t opcode, uint8_t op1, uint8_t op2);

    // implemented opcodes
    void op_nop   (uint8_t opcode, uint8_t op1, uint8_t op2);
    void op_mvi_a (uint8_t opcode, uint8_t op1, uint8_t op2);
    void op_cpi   (uint8_t opcode, uint8_t op1, uint8_t op2);
    void op_lxi_b (uint8_t opcode, uint8_t op1, uint8_t op2);
    void op_stax_b(uint8_t opcode, uint8_t op1, uint8_t op2);
};