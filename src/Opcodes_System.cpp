#include "Intel8080.h"
#include <iostream>

void Intel8080::doCall(uint16_t targetAddress) noexcept
{
    push16(PC); // Push current return PC address onto stack
    PC = targetAddress;
}

void Intel8080::doReturn() noexcept
{
    PC = pop16(); // Pull return address back into execution flow
}

void Intel8080::op_JmpCond(uint8_t opcode)
{
    uint16_t dest = getWord();

    // 0xC3 and 0xCB are unconditional JMP instructions
    if (opcode == 0b1100'0011 || opcode == 0b1100'1011)
    {
        PC = dest;
    }
    else
    {
        // Extract condition evaluation bits [5:3]
        if (checkCondition((opcode >> 3) & 0b0000'0111))
        {
            PC = dest;
        }
    }
}

void Intel8080::op_CallCond(uint8_t opcode)
{
    uint16_t dest = getWord();

    // 0xCD, 0xDD, 0xED, 0xFD all act as unconditional CALL. Clearing bits
    // [5:3] (the condition field) collapses all four down to 0b1100'0101.
    if ((opcode & 0b1100'0111) == 0b1100'0101)
    {
        doCall(dest);
    }
    else
    {
        if (checkCondition((opcode >> 3) & 0b0000'0111))
        {
            doCall(dest);
            // Conditional CALL costs 17 cycles when taken vs 11 when not;
            // OPCODE_TABLE stores the not-taken (base) cost.
            m_extraCycles = 6;
        }
    }
}

void Intel8080::op_RetCond(uint8_t opcode)
{
    // 0xC9 and 0xD9 act as unconditional RET
    if (opcode == 0b1100'1001 || opcode == 0b1101'1001)
    {
        doReturn();
    }
    else
    {
        if (checkCondition((opcode >> 3) & 0b0000'0111))
        {
            doReturn();
            // Conditional RET costs 11 cycles when taken vs 5 when not;
            // OPCODE_TABLE stores the not-taken (base) cost.
            m_extraCycles = 6;
        }
    }
}

void Intel8080::op_RST(uint8_t opcode)
{
    // Bits [5:3] represent the restart vector index.
    // Multiplying index by 8 gives the hardware jump target destination vector.
    uint16_t targetVector = static_cast<uint16_t>(opcode & 0b0011'1000);
    doCall(targetVector);
}

void Intel8080::op_HLT(uint8_t opcode)
{
    interrupts.halted = true;
}

void Intel8080::op_Interrupt(uint8_t opcode)
{
    // EI is 0xFB, DI is 0xF3. Bit 3 distinguishes them.
    interrupts.enabled = (opcode == 0b1111'1011);
}

void Intel8080::op_NOP(uint8_t opcode)
{
    // Do nothing loop anchor step
}

void Intel8080::op_IO(uint8_t opcode)
{
    uint8_t targetPort = getByte();

    if (opcode == 0b1101'0011)
    { // OUT (0xD3)
        m_bus.writePort(targetPort, regs.A);
    }
    else
    {                     // IN  (0xDB)
        regs.A = m_bus.readPort(targetPort);
    }
}

inline bool Intel8080::checkCondition(uint8_t cc) noexcept
{
    switch (cc)
    {
        case 0b000: return !flags.Zero;   // NZ (Not Zero)
        case 0b001: return flags.Zero;    // Z  (Zero)
        case 0b010: return !flags.Carry;  // NC (No Carry)
        case 0b011: return flags.Carry;   // C  (Carry)
        case 0b100: return !flags.Parity; // PO (Parity Odd)
        case 0b101: return flags.Parity;  // PE (Parity Even)
        case 0b110: return !flags.Sign;   // P  (Positive / Plus)
        case 0b111: return flags.Sign;    // M  (Minus / Negative)
    }
    return false;
}
