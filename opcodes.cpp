#include "opcodes.h"
#include <iostream>

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

void op_STA(uint8_t op1, uint8_t op2)
{
    // store A at 16-bit immediate address, no flags affected
    uint16_t addr = ((uint16_t)op2 << 8) | op1;
    memory[addr] = regs.A;
    PC += 3;
}

void op_LDA(uint8_t op1, uint8_t op2)
{
    // load A from 16-bit immediate address, no flags affected
    uint16_t addr = ((uint16_t)op2 << 8) | op1;
    regs.A = memory[addr];
    PC += 3;
}

void op_STAX(uint8_t hi, uint8_t lo)
{
    // store A at address held in register pair, no flags affected
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    memory[addr] = regs.A;
    PC += 1;
}

void op_LDAX(uint8_t hi, uint8_t lo)
{
    // load A from address held in register pair, no flags affected
    uint16_t addr = ((uint16_t)hi << 8) | lo;
    regs.A = memory[addr];
    PC += 1;
}

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

void op_SHLD(uint8_t op1, uint8_t op2)
{
    // store HL at address and address+1, no flags affected
    uint16_t addr = ((uint16_t)op2 << 8) | op1;
    memory[addr]     = regs.L;
    memory[addr + 1] = regs.H;
    PC += 3;
}

void op_LHLD(uint8_t op1, uint8_t op2)
{
    // load HL from address and address+1, no flags affected
    uint16_t addr = ((uint16_t)op2 << 8) | op1;
    regs.L = memory[addr];
    regs.H = memory[addr + 1];
    PC += 3;
}

void op_INR(uint8_t& reg)
{
    // CY is explicitly NOT touched by INR
    flags.AuxCarry = __calculateAuxCarryAdd(reg, 1);
    reg += 1;

    flags.Zero   = __calculateZero(reg);
    flags.Sign   = __calculateSign(reg);
    flags.Parity = __calculateParity(reg);

    PC += 1;
}

void op_DCR(uint8_t& reg)
{
    // CY is explicitly NOT touched by DCR
    flags.AuxCarry = __calculateAuxCarrySub(reg, 1);
    reg -= 1;

    flags.Zero   = __calculateZero(reg);
    flags.Sign   = __calculateSign(reg);
    flags.Parity = __calculateParity(reg);

    PC += 1;
}

void op_INX(uint16_t& reg16)
{
    // INX operates on a 16-bit register pair, no flags affected at all
    reg16 += 1;
    PC += 1;
}

void op_DCX(uint16_t& reg16)
{
    // DCX operates on a 16-bit register pair, no flags affected at all
    reg16 -= 1;
    PC += 1;
}

void op_DAD(uint16_t src)
{
    // 16-bit add: HL = HL + rp. Only CY is affected (carry out of bit 15)
    uint16_t hl = ((uint16_t)regs.H << 8) | regs.L;
    uint32_t result = (uint32_t)hl + src;
    flags.Carry = (result > 0xFFFF);
    regs.H = (result >> 8) & 0xFF;
    regs.L = result & 0xFF;
    PC += 1;
}



void op_DAA()
{
    // decimal adjust accumulator - fixes A after BCD arithmetic
    // this is the most complex instruction on the 8080

    uint8_t correction = 0;
    bool newCarry = flags.Carry;

    // if lower nibble > 9 or AC was set, add 6 to the lower nibble
    if ((regs.A & 0x0F) > 9 || flags.AuxCarry)
    {
        correction |= 0x06;
    }

    // if upper nibble > 9 or CY was set, add 0x60 to fix the upper nibble
    if ((regs.A >> 4) > 9 || flags.Carry
        || ((regs.A >> 4) == 9 && (regs.A & 0x0F) > 9))
    {
        correction |= 0x60;
        newCarry = true;
    }

    // AC = carry out of bit 3 from the correction add
    flags.AuxCarry = __calculateAuxCarryAdd(regs.A, correction);

    regs.A += correction;
    flags.Carry  = newCarry;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);

    PC += 1;
}


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

void op_RLC()
{
    // rotate A left, bit 7 goes to CY and wraps to bit 0
    // only CY is affected, Z/S/P/AC are not touched
    flags.Carry = (regs.A >> 7) & 1;
    regs.A = (regs.A << 1) | flags.Carry;
    PC += 1;
}

void op_RRC()
{
    // rotate A right, bit 0 goes to CY and wraps to bit 7
    flags.Carry = regs.A & 1;
    regs.A = (regs.A >> 1) | (flags.Carry << 7);
    PC += 1;
}

void op_RAL()
{
    // rotate A left through carry - old CY goes to bit 0, bit 7 becomes new CY
    uint8_t oldCarry = flags.Carry;
    flags.Carry = (regs.A >> 7) & 1;
    regs.A = (regs.A << 1) | oldCarry;
    PC += 1;
}

void op_RAR()
{
    // rotate A right through carry - old CY goes to bit 7, bit 0 becomes new CY
    uint8_t oldCarry = flags.Carry;
    flags.Carry = regs.A & 1;
    regs.A = (regs.A >> 1) | (oldCarry << 7);
    PC += 1;
}

// ─── accumulator/flag ops ────────────────────────────────────────────────────

void op_CMA()
{
    // complement accumulator - bitwise NOT of A, no flags affected
    regs.A = ~regs.A;
    PC += 1;
}

void op_STC()
{
    // set carry flag - only CY is affected
    flags.Carry = 1;
    PC += 1;
}

void op_CMC()
{
    // complement carry flag - flip CY, no other flags affected
    flags.Carry = !flags.Carry;
    PC += 1;
}

// ─── register pair / HL ops ──────────────────────────────────────────────────

void op_XCHG()
{
    // exchange DE and HL, no flags affected
    uint8_t tmpH = regs.H;
    uint8_t tmpL = regs.L;
    regs.H = regs.D;
    regs.L = regs.E;
    regs.D = tmpH;
    regs.E = tmpL;
    PC += 1;
}

void op_XTHL()
{
    // exchange HL with top of stack, no flags affected
    uint8_t tmpH = regs.H;
    uint8_t tmpL = regs.L;
    regs.L = memory[SP];
    regs.H = memory[SP + 1];
    memory[SP]     = tmpL;
    memory[SP + 1] = tmpH;
    PC += 1;
}

void op_PCHL()
{
    // jump to address in HL - no flags affected
    PC = ((uint16_t)regs.H << 8) | regs.L;
}

void op_SPHL()
{
    // copy HL into SP - no flags affected
    SP = ((uint16_t)regs.H << 8) | regs.L;
    PC += 1;
}

// ─── I/O and interrupt control ───────────────────────────────────────────────

uint16_t shift_register = 0;
uint8_t shift_offset = 0;

void op_OUT(uint8_t port)
{
    // The CPU wants to talk to hardware
    uint8_t value = regs.A; // OUT always sends from the Accumulator

    if (port == 2) {
        // Set the shift offset (only lower 3 bits matter)
        shift_offset = value & 0x07;
    }
    else if (port == 4) {
        // Shift data into the 16-bit register
        // Old high byte shifts to low byte, new data becomes high byte
        shift_register = (shift_register >> 8) | (value << 8);
    }

    PC += 2;
}

// Ensure you include your external port variables if they are in main.cpp
extern uint8_t port1;
extern uint8_t port2;

void op_IN(uint8_t port)
{
    // The CPU is requesting data from hardware, put it in the Accumulator
    if (port == 1) {
        regs.A = port1; // Read player 1 inputs
    }
    else if (port == 2) {
        regs.A = port2; // Read player 2/DIP switches
    }
    else if (port == 3) {
        // Read the result from the custom hardware shift register
        regs.A = (shift_register >> (8 - shift_offset)) & 0xFF;
    }
    else {
        regs.A = 0; // Default unconnected port
    }

    PC += 2;
}

void op_EI()
{
    // enable interrupts - INTE flag goes high after the *next* instruction
    interrupts.enabled = true;
    PC += 1;
}

void op_DI()
{
    // disable interrupts immediately
    interrupts.enabled = false;
    PC += 1;
}

void op_HLT()
{
    // halt - CPU stops until an interrupt arrives
    // for now we leave PC pointing at the HLT and let step() return false
    interrupts.halted = true;
}


void op_ANI(uint8_t op1)
{
    // 1. Calculate the AuxCarry based on the original Accumulator and immediate byte
    flags.AuxCarry = __calculateAuxCarryANI(regs.A, op1);

    // 2. Perform the actual bitwise AND operation
    regs.A = regs.A & op1;

    // 3. Update the remaining status flags based on the new register value
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Carry  = 0; // ANI always clears CY
    flags.Parity = __calculateParity(regs.A);

    PC += 2;
}

void op_ANA(uint8_t src)
{
    // 8080 ANA hardware quirk: AC is the logical OR of bit 3
    flags.AuxCarry = ((regs.A | src) & 0x08) != 0;
    flags.Carry    = 0;

    regs.A = regs.A & src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_XRA(uint8_t src)
{
    flags.AuxCarry = 0; // Explicitly 0
    flags.Carry    = 0;

    regs.A = regs.A ^ src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}



void op_XRI(uint8_t op1)
{
    regs.A = regs.A ^ op1;

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

void op_ORA(uint8_t src)
{
    flags.AuxCarry = 0; // Explicitly 0
    flags.Carry    = 0;

    regs.A = regs.A | src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_ORI(uint8_t op1)
{
    regs.A = regs.A | op1;

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

static void push16(uint16_t value)
{
    memory[SP - 1] = (value >> 8) & 0xFF;
    memory[SP - 2] = value & 0xFF;
    SP -= 2;
}

static uint16_t pop16()
{
    uint16_t lo = memory[SP];
    uint16_t hi = memory[SP + 1];
    SP += 2;
    return (hi << 8) | lo;
}

// ─── push/pop ────────────────────────────────────────────────────────────────

void op_PUSH(uint8_t hi, uint8_t lo)
{
    push16(((uint16_t)hi << 8) | lo);
    PC += 1;
}

void op_PUSH_PSW()
{
    // PSW = A in high byte, flags packed into low byte
    // bit layout: S Z 0 AC 0 P 1 CY
    uint8_t psw = (flags.Sign    << 7)
                | (flags.Zero    << 6)
                | (0             << 5)
                | (flags.AuxCarry<< 4)
                | (0             << 3)
                | (flags.Parity  << 2)
                | (1             << 1)   // bit 1 is always 1 on 8080
                | (flags.Carry   << 0);
    push16(((uint16_t)regs.A << 8) | psw);
    PC += 1;
}

void op_POP(uint8_t& hi, uint8_t& lo)
{
    uint16_t val = pop16();
    hi = (val >> 8) & 0xFF;
    lo = val & 0xFF;
    PC += 1;
}

void op_POP_PSW()
{
    uint16_t val = pop16();
    regs.A = (val >> 8) & 0xFF;
    uint8_t psw = val & 0xFF;

    // unpack flags from the PSW byte
    flags.Sign     = (psw >> 7) & 1;
    flags.Zero     = (psw >> 6) & 1;
    flags.AuxCarry = (psw >> 4) & 1;
    flags.Parity   = (psw >> 2) & 1;
    flags.Carry    = (psw >> 0) & 1;

    PC += 1;
}

// ─── call/return ─────────────────────────────────────────────────────────────

static void doCall(uint8_t op1, uint8_t op2)
{
    push16(PC + 3);
    PC = ((uint16_t)op2 << 8) | op1;
}

static void doReturn()
{
    PC = pop16();
}

void op_CALL(uint8_t op1, uint8_t op2) { doCall(op1, op2); }

void op_RET() { doReturn(); }

// conditional calls
void op_CNC(uint8_t op1, uint8_t op2) { if (!flags.Carry)  doCall(op1, op2); else PC += 3; }
void op_CNZ(uint8_t op1, uint8_t op2) { if (!flags.Zero)   doCall(op1, op2); else PC += 3; }
void op_CZ (uint8_t op1, uint8_t op2) { if ( flags.Zero)   doCall(op1, op2); else PC += 3; }
void op_CPE(uint8_t op1, uint8_t op2) { if ( flags.Parity) doCall(op1, op2); else PC += 3; }

// conditional returns
void op_RNC() { if (!flags.Carry)  doReturn(); else PC += 1; }
void op_RNZ() { if (!flags.Zero)   doReturn(); else PC += 1; }
void op_RC () { if ( flags.Carry)  doReturn(); else PC += 1; }
void op_RZ () { if ( flags.Zero)   doReturn(); else PC += 1; }
void op_RM () { if ( flags.Sign)   doReturn(); else PC += 1; }
void op_RP () { if (!flags.Sign)   doReturn(); else PC += 1; }
void op_RPE() { if ( flags.Parity) doReturn(); else PC += 1; }
void op_RPO() { if (!flags.Parity) doReturn(); else PC += 1; }

// RST n - restart: push PC then jump to vector at n*8
void op_RST(uint8_t vector)
{
    push16(PC + 1);
    PC = (uint16_t)vector * 8;
}

void op_CC (uint8_t op1, uint8_t op2) { if ( flags.Carry)  doCall(op1, op2); else PC += 3; }
void op_CPO(uint8_t op1, uint8_t op2) { if (!flags.Parity) doCall(op1, op2); else PC += 3; }
void op_CM (uint8_t op1, uint8_t op2) { if ( flags.Sign)   doCall(op1, op2); else PC += 3; }
void op_CP (uint8_t op1, uint8_t op2) { if (!flags.Sign)   doCall(op1, op2); else PC += 3; }

// ─── ADDITION ────────────────────────────────────────────────────────────────

void op_ADD(uint8_t src)
{
    uint16_t result = (uint16_t)regs.A + src;

    flags.AuxCarry = ((regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_ADI(uint8_t op1)
{
    uint16_t result = (uint16_t)regs.A + op1;

    flags.AuxCarry = ((regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

void op_ADC(uint8_t src)
{
    uint8_t carryIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A + src + carryIn;

    flags.AuxCarry = ((regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_ACI(uint8_t op1)
{
    uint8_t carryIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A + op1 + carryIn;

    flags.AuxCarry = ((regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

// ─── SUBTRACTION & BORROW ────────────────────────────────────────────────────

void op_SUB(uint8_t src)
{
    uint16_t result = (uint16_t)regs.A - src;

    // Subtraction uses inverted XOR sum to track 8080 hardware adder carries
    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < src);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_SUI(uint8_t op1)
{
    uint16_t result = (uint16_t)regs.A - op1;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < op1);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

void op_SBB(uint8_t src)
{
    uint8_t borrowIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A - src - borrowIn;

    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < (uint16_t)src + borrowIn);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 1;
}

void op_SBI(uint8_t op1)
{
    uint8_t borrowIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A - op1 - borrowIn;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < (uint16_t)op1 + borrowIn);

    regs.A = (uint8_t)result;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
    PC += 2;
}

// ─── COMPARISONS ─────────────────────────────────────────────────────────────

void op_CMP(uint8_t src)
{
    uint16_t result = (uint16_t)regs.A - src;

    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < src);

    uint8_t res8 = (uint8_t)result;
    flags.Zero   = __calculateZero(res8);
    flags.Sign   = __calculateSign(res8);
    flags.Parity = __calculateParity(res8);
    PC += 1;
}

void op_CPI(uint8_t op1)
{
    uint16_t result = (uint16_t)regs.A - op1;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < op1);

    uint8_t res8 = (uint8_t)result;
    flags.Zero   = __calculateZero(res8);
    flags.Sign   = __calculateSign(res8);
    flags.Parity = __calculateParity(res8);
    PC += 2;
}
