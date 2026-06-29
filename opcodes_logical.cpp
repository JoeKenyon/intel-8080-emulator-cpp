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
