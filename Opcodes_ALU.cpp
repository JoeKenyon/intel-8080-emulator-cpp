#include "Intel8080.h"
#include <bit>

inline void Intel8080::executeAdd(uint8_t value) noexcept
{
    uint16_t result = static_cast<uint16_t>(regs.A) + value;

    // Inline Addition AC: Carry out of bit 3
    flags.AuxCarry = ((regs.A & 0b0000'1111) + (value & 0b0000'1111)) > 0b0000'1111;
    flags.Carry = (result > 0b1111'1111);
    regs.A = static_cast<uint8_t>(result & 0b1111'1111);
    setAluFlags(regs.A);
}

inline void Intel8080::executeAdc(uint8_t value) noexcept
{
    uint8_t cy = flags.Carry ? 0b0000'0001 : 0b0000'0000;
    uint16_t result = static_cast<uint16_t>(regs.A) + value + cy;

    flags.AuxCarry = ((regs.A & 0b0000'1111) + (value & 0b0000'1111) + cy) > 0b0000'1111;
    flags.Carry = (result > 0b1111'1111);
    regs.A = static_cast<uint8_t>(result & 0b1111'1111);
    setAluFlags(regs.A);
}

inline void Intel8080::executeSub(uint8_t value) noexcept
{
    uint16_t result = static_cast<uint16_t>(regs.A) - value;

    // Accurate 8080 subtraction borrow tracking
    flags.AuxCarry = !((regs.A & 0x0F) < (value & 0x0F));
    flags.Carry = (regs.A < value);
    regs.A = static_cast<uint8_t>(result & 0xFF);
    setAluFlags(regs.A);
}

inline void Intel8080::executeSbb(uint8_t value) noexcept
{
    uint8_t borrow = flags.Carry ? 1 : 0;
    int result = static_cast<int>(regs.A) - static_cast<int>(value) - borrow;

    // Accurate 8080 half-carry / borrow logic for SBB
    flags.AuxCarry = !((static_cast<int>(regs.A & 0x0F) - static_cast<int>(value & 0x0F) - borrow) < 0);

    // Full carry is set if the overall result underflows 0
    flags.Carry = (result < 0);

    regs.A = static_cast<uint8_t>(result & 0xFF);
    setAluFlags(regs.A);
}

inline void Intel8080::executeAna(uint8_t value) noexcept
{
    // The infamous 8080 ANA AuxCarry logic:
    flags.AuxCarry = ((regs.A | value) & 0x08) != 0;
    flags.Carry = false;
    regs.A &= value;
    setAluFlags(regs.A);
}

inline void Intel8080::executeXra(uint8_t value) noexcept
{
    flags.AuxCarry = false;
    flags.Carry = false;
    regs.A ^= value;
    setAluFlags(regs.A);
}

inline void Intel8080::executeOra(uint8_t value) noexcept
{
    flags.AuxCarry = false;
    flags.Carry = false;
    regs.A |= value;
    setAluFlags(regs.A);
}

inline void Intel8080::executeCmp(uint8_t value) noexcept
{
    uint16_t result = static_cast<uint16_t>(regs.A) - value;

    // Accurate 8080 subtraction borrow tracking
    flags.AuxCarry = !((regs.A & 0x0F) < (value & 0x0F));
    flags.Carry = (regs.A < value);
    setAluFlags(static_cast<uint8_t>(result & 0xFF));
}

inline void Intel8080::setAluFlags(uint8_t result) noexcept
{
    flags.Zero = calculateZero(result);
    flags.Sign = calculateSign(result);
    flags.Parity = calculateParity(result);
}

constexpr bool Intel8080::calculateParity(uint8_t result) noexcept
{
    return (std::popcount(result) % 2) == 0;
}

void Intel8080::op_AluReg(uint8_t opcode)
{
    uint8_t src = opcode & 0b0000'0111;
    uint8_t type = (opcode >> 3) & 0b0000'0111;
    uint8_t val = (src == 0b0000'0110) ? m_bus.readMemory(regs.HL) : decodeReg(src);

    switch (type)
    {
        case 0b000: executeAdd(val); break; // ADD
        case 0b001: executeAdc(val); break; // ADC
        case 0b010: executeSub(val); break; // SUB
        case 0b011: executeSbb(val); break; // SBB
        case 0b100: executeAna(val); break; // ANA
        case 0b101: executeXra(val); break; // XRA
        case 0b110: executeOra(val); break; // ORA
        case 0b111: executeCmp(val); break; // CMP
    }
}

void Intel8080::op_AluImm(uint8_t opcode)
{
    uint8_t type = (opcode >> 3) & 0b0000'0111;
    uint8_t val = getByte();

    switch (type)
    {
        case 0b000: executeAdd(val); break; // ADI
        case 0b001: executeAdc(val); break; // ACI
        case 0b010: executeSub(val); break; // SUI
        case 0b011: executeSbb(val); break; // SBI
        case 0b100: executeAna(val); break; // ANI
        case 0b101: executeXra(val); break; // XRI
        case 0b110: executeOra(val); break; // ORI
        case 0b111: executeCmp(val); break; // CPI
    }
}

void Intel8080::op_INR_DCR(uint8_t opcode)
{
    uint8_t reg = (opcode >> 3) & 0b0000'0111;
    bool isDcr = (opcode & 0b0000'0001) != 0b0000'0000;
    uint8_t val = (reg == 0b0000'0110) ? m_bus.readMemory(regs.HL) : decodeReg(reg);

    if (isDcr)
    {
        flags.AuxCarry = ((val & 0b0000'1111) + (~0b0000'0001 & 0b0000'1111) + 0b0000'0001) > 0b0000'1111;
        val--;
    }
    else
    {
        flags.AuxCarry = ((val & 0b0000'1111) + 0b0000'0001) > 0b0000'1111;
        val++;
    }

    setAluFlags(val); // Note: INR/DCR purposefully do NOT affect the Carry Flag!
    if (reg == 0b0000'0110) m_bus.writeMemory(regs.HL, val);
    else decodeReg(reg) = val;
}

void Intel8080::op_INX_DCX(uint8_t opcode)
{
    uint8_t pair = (opcode >> 4) & 0b0000'0011;
    bool isDcx = (opcode & 0b0000'1000) != 0b0000'0000;
    int16_t delta = isDcx ? -1 : 1;

    switch (pair)
    {
        case 0b00: regs.BC += delta; break;
        case 0b01: regs.DE += delta; break;
        case 0b10: regs.HL += delta; break;
        case 0b11: SP += delta;      break;
    }
}

void Intel8080::op_DAD(uint8_t opcode)
{
    uint8_t pair = (opcode >> 4) & 0b0000'0011;
    uint32_t src = 0b0000'0000;

    switch (pair)
    {
        case 0b00: src = regs.BC; break;
        case 0b01: src = regs.DE; break;
        case 0b10: src = regs.HL; break;
        case 0b11: src = SP;      break;
    }

    uint32_t res = static_cast<uint32_t>(regs.HL) + src;
    flags.Carry = (res > 0b1111'1111'1111'1111);
    regs.HL = static_cast<uint16_t>(res & 0b1111'1111'1111'1111);
}

void Intel8080::op_Rotate(uint8_t opcode)
{
    uint8_t type = (opcode >> 3) & 0b0000'0011;

    switch (type)
    {
        case 0b00:
        { // RLC
            flags.Carry = (regs.A >> 7) & 0b0000'0001;
            regs.A = std::rotl(regs.A, 1);
            break;
        }
        case 0b01: // RRC
        {
            flags.Carry = regs.A & 0b0000'0001;
            regs.A = std::rotr(regs.A, 1);
            break;
        }
        case 0b10:
        { // RAL
            uint8_t oldCy = flags.Carry ? 0b0000'0001 : 0b0000'0000;
            flags.Carry = (regs.A >> 7) & 0b0000'0001;
            regs.A = (regs.A << 1) | oldCy;
            break;
        }
        case 0b11:
        { // RAR
            uint8_t oldCy = flags.Carry ? 0b0000'0001 : 0b0000'0000;
            flags.Carry = regs.A & 0b0000'0001;
            regs.A = (regs.A >> 1) | (oldCy << 7);
            break;
        }
    }
}

void Intel8080::op_AccControl(uint8_t opcode)
{
    switch (opcode)
    {
        case 0b0010'1111: regs.A = ~regs.A; break; // CMA (0x2F)
        case 0b0011'0111: flags.Carry = true;  break; // STC (0x37)
        case 0b0011'1111: flags.Carry = !flags.Carry; break; // CMC (0x3F)
        case 0b0010'0111: // DAA (0x27)
        {
            uint8_t correction = 0b0000'0000;
            bool newCy = flags.Carry;

            if ((regs.A & 0b0000'1111) > 9 || flags.AuxCarry)
            {
                correction |= 0b0000'0110;
            }
            if ((regs.A >> 4) > 9 || flags.Carry || ((regs.A >> 4) == 9 && (regs.A & 0b0000'1111) > 9))
            {
                correction |= 0b0110'0000;
                newCy = true;
            }

            flags.AuxCarry = ((regs.A & 0b0000'1111) + (correction & 0b0000'1111)) > 0b0000'1111;
            regs.A += correction;
            flags.Carry = newCy;
            setAluFlags(regs.A);
            break;
        }
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
