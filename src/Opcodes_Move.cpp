#include "Intel8080.h"
#include <utility>

void Intel8080::push16(uint16_t value) noexcept
{
    SP -= 2;
    m_bus.writeMemoryWord(SP, value);
}

uint16_t Intel8080::pop16() noexcept
{
    uint16_t val = m_bus.readMemoryWord(SP);
    SP += 2;
    return val;
}

void Intel8080::op_MOV(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0b0000'0111;
    uint8_t src = opcode & 0b0000'0111;

    // Index 6 (0b110) points directly to memory at HL
    uint8_t val = (src == 0b0000'0110) ? m_bus.readMemory(regs.HL) : decodeReg(src);

    if (dst == 0b0000'0110)
    {
        m_bus.writeMemory(regs.HL, val);
    }
    else
    {
        decodeReg(dst) = val;
    }
}

void Intel8080::op_MVI(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0b0000'0111;
    uint8_t val = getByte();

    if (dst == 0b0000'0110)
    {
        m_bus.writeMemory(regs.HL, val);
    }
    else
    {
        decodeReg(dst) = val;
    }
}

void Intel8080::op_LXI(uint8_t opcode)
{
    uint8_t pair = (opcode >> 4) & 0b0000'0011;
    uint16_t val = getWord();

    switch (pair)
    {
        case 0b00: regs.BC = val; break;
        case 0b01: regs.DE = val; break;
        case 0b10: regs.HL = val; break;
        case 0b11: SP = val;      break;
    }
}

void Intel8080::op_STAX_LDAX(uint8_t opcode)
{
    uint16_t addr = ((opcode & 0b0001'0000) == 0b0000'0000) ? regs.BC : regs.DE;

    // STAX opcodes have bit 3 cleared (e.g., 0x02, 0x12)
    if ((opcode & 0b0000'1000) == 0b0000'0000)
    {
        m_bus.writeMemory(addr, regs.A);
    }
    else
    {
        regs.A = m_bus.readMemory(addr);
    }
}

void Intel8080::op_Direct16(uint8_t opcode)
{
    uint16_t addr = getWord();

    switch (opcode)
    {
        case 0b0011'0010: m_bus.writeMemory(addr, regs.A); break;      // STA  (0x32)
        case 0b0011'1010: regs.A = m_bus.readMemory(addr); break;      // LDA  (0x3A)
        case 0b0010'0010: m_bus.writeMemoryWord(addr, regs.HL); break; // SHLD (0x22)
        case 0b0010'1010: regs.HL = m_bus.readMemoryWord(addr); break; // LHLD (0x2A)
    }
}

void Intel8080::op_PUSH(uint8_t opcode)
{
    uint8_t pair = (opcode >> 4) & 0b0000'0011;

    switch (pair)
    {
        case 0b00: push16(regs.BC); break;
        case 0b01: push16(regs.DE); break;
        case 0b10: push16(regs.HL); break;
        case 0b11:
        { // PUSH PSW (Accumulator + Processor Status Word flags)
            uint8_t psw = (static_cast<uint8_t>(flags.Sign)   << 7) |
                          (static_cast<uint8_t>(flags.Zero)   << 6) |
                          (0b0                                << 5) |
                          (static_cast<uint8_t>(flags.AuxCarry) << 4) |
                          (0b0                                << 3) |
                          (static_cast<uint8_t>(flags.Parity) << 2) |
                          (0b1                                << 1) | // Bit 1 is hardwired to 1
                          (static_cast<uint8_t>(flags.Carry)  << 0);
            push16((static_cast<uint16_t>(regs.A) << 8) | psw);
            break;
        }
    }
}

void Intel8080::op_POP(uint8_t opcode)
{
    uint8_t pair = (opcode >> 4) & 0b0000'0011;
    uint16_t val = pop16();

    switch (pair)
    {
        case 0b00: regs.BC = val; break;
        case 0b01: regs.DE = val; break;
        case 0b10: regs.HL = val; break;
        case 0b11:
        { // POP PSW
            regs.A = (val >> 8) & 0b1111'1111;
            uint8_t psw = val & 0b1111'1111;

            flags.Sign     = (psw >> 7) & 0b0000'0001;
            flags.Zero     = (psw >> 6) & 0b0000'0001;
            flags.AuxCarry = (psw >> 4) & 0b0000'0001;
            flags.Parity   = (psw >> 2) & 0b0000'0001;
            flags.Carry    = (psw >> 0) & 0b0000'0001;
            break;
        }
    }
}

void Intel8080::op_XCHG_XTHL(uint8_t opcode)
{
    if (opcode == 0b1110'1011)
    {   // XCHG (0xEB)
        std::swap(regs.DE, regs.HL);
    }
    else
    {   // XTHL (0xE3)
        uint16_t temp = regs.HL;
        regs.HL = m_bus.readMemoryWord(SP);
        m_bus.writeMemoryWord(SP, temp);
    }
}

void Intel8080::op_PCHL_SPHL(uint8_t opcode)
{
    if (opcode == 0b1110'1001)
    { // PCHL (0xE9)
        PC = regs.HL;
    }
    else
    { // SPHL (0xF9)
        SP = regs.HL;
    }
}
