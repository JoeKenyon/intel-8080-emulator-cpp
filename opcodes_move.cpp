#include "opcodes.h"
#include "hardware.h"

void op_LXI_SP(uint8_t op1, uint8_t op2)
{
    uint16_t immediateValue = (op2 << 8) | op1;
    SP = immediateValue;
    PC += 3;
}

void op_LXI_Reg(uint8_t& regHigh, uint8_t& regLow, uint8_t op1, uint8_t op2)
{
    regLow = op1;
    regHigh = op2;
    PC += 3;
}

// move src value into destination, from reg
void op_MOV(uint8_t& dest, uint8_t src)
{
    dest = src;
    PC += 1;
}

// move immediate value into destination
void op_MVI(uint8_t& dest, uint8_t op1)
{
    dest = op1;
    PC += 2;
}
