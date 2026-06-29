#include "opcodes.h"
#include "hardware.h"

void op_JMP(uint8_t op1, uint8_t op2)
{
    // Intel 8080 is little-endian: low-byte (op1) then high-byte (op2)
    uint16_t targetAddress = (op2 << 8) | op1;

    PC = targetAddress; // move the PC to point to where
}

void op_JC(uint8_t op1, uint8_t op2)
{
    if (flags.Carry)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Jump skipped, advance past this 3-byte instruction
    }
}

void op_JNC(uint8_t op1, uint8_t op2)
{
    if (!flags.Carry)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Jump skipped, advance past this 3-byte instruction
    }
}

void op_JZ(uint8_t op1, uint8_t op2)
{
    if(flags.Zero)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JZ instruction cleanly
    }
}

void op_JNZ(uint8_t op1, uint8_t op2)
{
    if(!flags.Zero)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JNZ instruction cleanly
    }
}

void op_JPE(uint8_t op1, uint8_t op2)
{
    if(flags.Parity)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JNZ instruction cleanly
    }
}

void op_JPO(uint8_t op1, uint8_t op2)
{
    if(!flags.Parity)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JNZ instruction cleanly
    }
}

void op_JM(uint8_t op1, uint8_t op2)
{
    if(flags.Sign)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JNZ instruction cleanly
    }
}

void op_JP(uint8_t op1, uint8_t op2)
{
    if(!flags.Sign)
    {
        uint16_t targetAddress = (op2 << 8) | op1;
        PC = targetAddress;
    }
    else
    {
        PC += 3; // Condition failed, skip over the 3-byte JNZ instruction cleanly
    }
}
