#include "opcodes.h"
#include "hardware.h"


void op_ANI(uint8_t op1)
{
    regs.A = regs.A & op1; // use op1

    // update flags based on register value
    flags.Zero     = __calculateZero(regs.A);
    flags.Sign     = __calculateSign(regs.A);
    flags.Carry    = 0;
    flags.Parity   = __calculateParity(regs.A);
    flags.AuxCarry = __calculateAuxCarryANI(op1);

    PC += 2;
}

void op_CMP(uint8_t src)
{
    // evaluate borrow flags using the source value
    flags.AuxCarry = __calculateAuxCarrySub(regs.A, src);
    flags.Carry    = (regs.A < src);

    uint8_t virtualResult = regs.A - src;

    flags.Zero   = __calculateZero(virtualResult);
    flags.Sign   = __calculateSign(virtualResult);
    flags.Parity = __calculateParity(virtualResult);

    // CMP instructions are all 1 byte long
    PC += 1;
}
