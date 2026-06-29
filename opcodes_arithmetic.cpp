#include "opcodes.h"
#include "hardware.h"


void op_ADI(uint8_t op1)
{
    // add immediate
    // calculate aux carry befor they change
    flags.AuxCarry = __calculateAuxCarryAdd(regs.A, op1);
    uint16_t result = (uint16_t)regs.A + op1;
    flags.Carry = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);

    PC += 2;
}
void op_ADD(uint8_t src)
{
    // add immediate
    // calculate aux carry befor they change
    flags.AuxCarry = __calculateAuxCarryAdd(regs.A, src);
    uint16_t result = (uint16_t)regs.A + src;
    flags.Carry = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);

    PC += 1;
}
void op_SUI(uint8_t op1)
{
    flags.AuxCarry = __calculateAuxCarrySub(regs.A, op1);
    flags.Carry = (regs.A < op1);
    regs.A = regs.A - op1;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);

    PC += 2;
}
void op_SUB(uint8_t src)
{
    flags.AuxCarry = __calculateAuxCarrySub(regs.A, src);
    flags.Carry = (regs.A < src);
    regs.A = regs.A - src;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);

    PC += 1;
}
void op_INR(uint8_t& reg)
{
    // calculate Auxiliary Carry out of bit 3 before the value increments
    flags.AuxCarry = __calculateAuxCarryAdd(reg, 1);
    reg += 1;

    // Update status flags, carry flag left alone
    flags.Zero   = __calculateZero(reg);
    flags.Sign   = __calculateSign(reg);
    flags.Parity = __calculateParity(reg);

    // INR is a 1-byte instruction
    PC += 1;
}

void op_DCR(uint8_t& reg)
{
    // calculate Auxiliary Carry out of bit 3 before the value chanegs
    flags.AuxCarry = __calculateAuxCarrySub(reg, 1);
    reg -= 1;

    // Update status flags, carry flag left alone
    flags.Zero   = __calculateZero(reg);
    flags.Sign   = __calculateSign(reg);
    flags.Parity = __calculateParity(reg);

    PC += 1;
}
