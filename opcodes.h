#pragma once
#include "hardware.h"
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

// --- data movement ---
void op_MOV(uint8_t& dest, uint8_t src);
void op_MVI(uint8_t& dest, uint8_t op1);
void op_STA(uint8_t op1, uint8_t op2);
void op_LDA(uint8_t op1, uint8_t op2);
void op_STAX(uint8_t hi, uint8_t lo);
void op_LDAX(uint8_t hi, uint8_t lo);
void op_LXI_SP(uint8_t op1, uint8_t op2);
void op_LXI_Reg(uint8_t& regHigh, uint8_t& regLow, uint8_t op1, uint8_t op2);
void op_SHLD(uint8_t op1, uint8_t op2);
void op_LHLD(uint8_t op1, uint8_t op2);

// --- arithmetic ---
void op_ADD(uint8_t src);
void op_ADI(uint8_t op1);
void op_ADC(uint8_t src);
void op_ACI(uint8_t op1);
void op_SUB(uint8_t src);
void op_SUI(uint8_t op1);
void op_SBB(uint8_t src);
void op_SBI(uint8_t op1);
void op_INR(uint8_t& reg);
void op_DCR(uint8_t& reg);
void op_INX(uint16_t& regPair);
void op_DCX(uint16_t& regPair);
void op_DAD(uint16_t val);

// --- logical ---
void op_ANA(uint8_t src);
void op_ANI(uint8_t op1);
void op_XRA(uint8_t src);
void op_XRI(uint8_t op1);
void op_ORA(uint8_t src);
void op_ORI(uint8_t op1);
void op_CMP(uint8_t src);
void op_CPI(uint8_t op1);
void op_RLC();
void op_RRC();
void op_RAL();
void op_RAR();
void op_CMA();
void op_STC();
void op_CMC();
void op_DAA();

// --- program flow / branches ---
void op_JMP(uint8_t op1, uint8_t op2);
void op_JC(uint8_t op1, uint8_t op2);
void op_JNC(uint8_t op1, uint8_t op2);
void op_JZ(uint8_t op1, uint8_t op2);
void op_JNZ(uint8_t op1, uint8_t op2);
void op_JM(uint8_t op1, uint8_t op2);
void op_JP(uint8_t op1, uint8_t op2);
void op_JPE(uint8_t op1, uint8_t op2);
void op_JPO(uint8_t op1, uint8_t op2);
void op_CALL(uint8_t op1, uint8_t op2);
void op_C(uint8_t op1, uint8_t op2);
void op_CC(uint8_t op1, uint8_t op2);
void op_CNC(uint8_t op1, uint8_t op2);
void op_CZ(uint8_t op1, uint8_t op2);
void op_CNZ(uint8_t op1, uint8_t op2);
void op_CM(uint8_t op1, uint8_t op2);
void op_CP(uint8_t op1, uint8_t op2);
void op_CPE(uint8_t op1, uint8_t op2);
void op_CPO(uint8_t op1, uint8_t op2);
void op_RET();
void op_RC();
void op_RNC();
void op_RZ();
void op_RNZ();
void op_RM();
void op_RP();
void op_RPE();
void op_RPO();
void op_RST(uint8_t vector);

// --- stack / address transfers ---
void op_PUSH(uint8_t hi, uint8_t lo);
void op_PUSH_PSW();
void op_POP(uint8_t& hi, uint8_t& lo);
void op_POP_PSW();
void op_XCHG();
void op_XTHL();
void op_PCHL();
void op_SPHL();

// --- system execution & io ---
void op_HLT();
void op_EI();
void op_DI();
void op_NOP();
void op_OUT(uint8_t port);
void op_IN(uint8_t port);
