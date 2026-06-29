#pragma once
#include <cstdint>

// set zero flag if result is 0 lol
inline bool __calculateZero(uint8_t result)
{
    return result == 0;
}

// returns true if the top bit is set
inline bool __calculateSign(uint8_t result)
{
    return (result & 0x80) != 0;
}

// returns true if the number of set bits is even
inline bool __calculateParity(uint8_t result)
{
    uint8_t p = result;
    p ^= p >> 4;
    p ^= p >> 2;
    p ^= p >> 1;
    return !(p & 1);
}

// For ANI d8: Extract bit 3 of the immediate data byte
inline bool __calculateAuxCarryANI(uint8_t op1)
{
    return (op1 & 0x08) != 0;
}

// For ANA r: Logical OR of bit 3 from both registers
inline bool __calculateAuxCarryANA(uint8_t a, uint8_t b)
{
    return ((a | b) & 0x08) != 0;
}

// Returns true if addition carries out of bit 3 (lower nibble overflow)
inline bool __calculateAuxCarryAdd(uint8_t a, uint8_t b)
{
    return (((a & 0x0F) + (b & 0x0F)) > 0x0F);
}

// returns true if subtraction borrows into bit 3 (lower nibble underflow)
inline bool __calculateAuxCarrySub(uint8_t a, uint8_t b)
{
    return ((a & 0x0F) < (b & 0x0F));
}

void op_LXI_SP(uint8_t op1, uint8_t op2);
void op_LXI_Reg(uint8_t& regHigh, uint8_t& regLow, uint8_t op1, uint8_t op2);
void op_MOV(uint8_t& dest, uint8_t src);
void op_MVI(uint8_t& dest, uint8_t op1);

void op_ADI(uint8_t op1);
void op_ADD(uint8_t src);
void op_SUI(uint8_t op1);
void op_SUB(uint8_t src);
void op_INR(uint8_t& reg);
void op_DCR(uint8_t& reg);

void op_ANI(uint8_t op1);
void op_ANA(uint8_t src);
void op_XRA(uint8_t src);
void op_ORA(uint8_t src);

void op_JMP(uint8_t op1, uint8_t op2);
void op_JC(uint8_t op1, uint8_t op2);
void op_JNC(uint8_t op1, uint8_t op2);
