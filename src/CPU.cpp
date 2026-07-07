#include <iostream>
#include <utility>
#include "CPU.h"

CPU::CPU(IBus& b) : bus(b)
{
}

void CPU::requestInterrupt(uint8_t vector)
{
    // if the game called di() to disable interrupts, ignore the hardware
    if (!systemFlags.interruptEnabled)
    {
        return;
    }

    // the moment an interrupt fires, the cpu automatically acts like di() was called.
    // the game must manually call ei() before it can receive the next half-frame interrupt.
    systemFlags.interruptEnabled = false;

    // if the cpu was sleeping via hlt(), the interrupt wakes it back up
    systemFlags.halted = false;

    // perform the exact same logic as your software rst() handler
    pushWord(PC);
    PC = vector;
}

int CPU::executeInstruction()
{
    uint8_t opcode = fetchByte(); // PC is now (currentPC + 1)
    currentOpcode = opcode;       // field-decoded handlers (MOV/INR/ALU/RST) read this
    const Instruction& instr = OPCODE_TABLE[opcode];

    // reset extra cycles
    m_extraCycles = 0;

    // call the handler
    (this->*instr.handler)();

    return instr.cycles  + m_extraCycles;
}

uint8_t& CPU::getReg8(uint8_t code)
{
    switch (code & 0x7)
    {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 7: return A;
        default: return A; // Code 6 is M, handled by specific M helpers
    }
}

// Logic Operations
void CPU::alu_and(uint8_t val)
{
    statusFlags.AuxCarry = ((A | val) & 0x08) != 0;
    statusFlags.Carry = 0;
    A &= val;
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_xor(uint8_t val)
{
    statusFlags.AuxCarry = 0;
    statusFlags.Carry = 0;
    A ^= val;
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_or(uint8_t val)
{
    statusFlags.AuxCarry = 0;
    statusFlags.Carry = 0;
    A |= val;
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

// Arithmetic Operations
void CPU::alu_add(uint8_t val)
{
    uint16_t tmp = A + val;
    setHalfCarryAdd(A, val, tmp);
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_adc(uint8_t val)
{
    uint16_t tmp = A + val + statusFlags.Carry;
    setHalfCarryAdd(A, val, tmp);
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_sub(uint8_t val)
{
    uint16_t tmp = A - val;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp); // Note: 8080 Carry flag is set on borrow
    A = (uint8_t)(tmp & 0xFF);
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_sbb(uint8_t val)
{
    uint16_t tmp = A - val - statusFlags.Carry;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZeroFlag(A);
    setSignFlag(A);
    setParityFlag(A);
}

void CPU::alu_cmp(uint8_t val)
{
    uint16_t tmp = A - val;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp);
    setZeroFlag((uint8_t)(tmp & 0xFF));
    setSignFlag((uint8_t)(tmp & 0xFF));
    setParityFlag((uint8_t)(tmp & 0xFF));
}

void CPU::pushByte(uint8_t value)
{
    bus.writeByte(--SP, value);
}

uint8_t CPU::popByte()
{
    return bus.readByte(SP++);
}

void CPU::pushWord(uint16_t value)
{
    // 8080 pushes the high byte first, then the low byte
    bus.writeByte(--SP, (value >> 8) & 0xFF);
    bus.writeByte(--SP, value & 0xFF);
}

uint16_t CPU::popWord()
{
    // 8080 pops the low byte first, then the high byte
    uint16_t low  = bus.readByte(SP++);
    uint16_t high = bus.readByte(SP++);
    return (high << 8) | low;
}

// centralized call logic
void CPU::doCall(bool condition)
{
    // fetch the target address using the word helper
    // this moves the pc forward by 2 bytes regardless of the jump
    uint16_t addr = fetchWord();

    // if the condition is met, perform the call
    if (condition)
    {
        // save the return spot and jump to the target
        pushWord(PC);
        PC = addr;
        // conditionals add 6 cycles apparently
        m_extraCycles = 6;
    }
}

// unconditional call
void CPU::call() { uint16_t addr = fetchWord(); pushWord(PC); PC = addr; }
void CPU::cc()   { doCall(statusFlags.Carry); }
void CPU::cnc()  { doCall(!statusFlags.Carry); }
void CPU::cz()   { doCall(statusFlags.Zero); }
void CPU::cnz()  { doCall(!statusFlags.Zero); }
void CPU::cp()   { doCall(!statusFlags.Sign); }
void CPU::cm()   { doCall(statusFlags.Sign); }
void CPU::cpe()  { doCall(statusFlags.Parity); }
void CPU::cpo()  { doCall(!statusFlags.Parity); }

void CPU::doReturn(bool condition)
{
    // if the condition is met, pop the return address
    if (condition)
    {
        PC = popWord();
        // conditionals add 6 cycles apparently
        m_extraCycles = 6;
    }
}

void CPU::ret() { PC = popWord(); }
void CPU::rc()  { doReturn(statusFlags.Carry); }
void CPU::rnc() { doReturn(!statusFlags.Carry); }
void CPU::rz()  { doReturn(statusFlags.Zero); }
void CPU::rnz() { doReturn(!statusFlags.Zero); }
void CPU::rp()  { doReturn(!statusFlags.Sign); }
void CPU::rm()  { doReturn(statusFlags.Sign); }
void CPU::rpe() { doReturn(statusFlags.Parity); }
void CPU::rpo() { doReturn(!statusFlags.Parity); }

void CPU::doJump(bool condition)
{
    // grab the jump address regardless of condition
    // this moves the pc forward by 2 bytes (3 total including opcode)
    uint16_t addr = fetchWord();

    // if the condition is met, perform the jump
    if (condition)
    {
        PC = addr;
    }
}

void CPU::jmp() { doJump(true); }
void CPU::jc()  { doJump(statusFlags.Carry); }
void CPU::jnc() { doJump(!statusFlags.Carry); }
void CPU::jz()  { doJump(statusFlags.Zero); }
void CPU::jnz() { doJump(!statusFlags.Zero); }
void CPU::jp()  { doJump(!statusFlags.Sign); }
void CPU::jm()  { doJump(statusFlags.Sign); }
void CPU::jpe() { doJump(statusFlags.Parity); }
void CPU::jpo() { doJump(!statusFlags.Parity); }

void CPU::in()
{
    // grab the port number from the next byte
    uint8_t port = fetchByte();

    // read from the port via the bus
    A = bus.readPort(port);
}

void CPU::out()
{
    // grab the port number from the next byte
    uint8_t port = fetchByte();

    // write the accumulator to the port via the bus
    bus.writePort(port, A);
}

void CPU::lxiBC()
{
    // fetchword handles the 16-bit read and advances pc
    BC = fetchWord();
}

void CPU::lxiDE()
{
    DE = fetchWord();
}

void CPU::lxiHL()
{
    HL = fetchWord();
}

void CPU::lxiSP()
{
    // stack pointer is already 16-bit
    SP = fetchWord();
}

void CPU::pushBC()
{
    pushWord(BC);
}

void CPU::pushDE()
{
    pushWord(DE);
}

void CPU::pushHL()
{
    pushWord(HL);
}

void CPU::pushPSW()
{
    // update the fixed status bits in the flags struct
    statusFlags._zero1 = 0;
    statusFlags._zero2 = 0;
    statusFlags._one   = 1;

    // push the whole pair (a + statusFlags.psw) automatically
    pushWord(PSW);
}

void CPU::popBC()
{
    BC = popWord();
}

void CPU::popDE()
{
    DE = popWord();

}
void CPU::popHL()
{
    HL = popWord();
}

void CPU::popPSW()
{
    // pop the word directly into the psw pair (a + statusFlags.psw)
    PSW = popWord();

    // restore the fixed status bits that might have been overwritten
    statusFlags._zero1 = 0;
    statusFlags._zero2 = 0;
    statusFlags._one   = 1;
}

void CPU::sta()
{
    // grab the destination address from the stream
    uint16_t addr = fetchWord();

    // write the accumulator to memory
    bus.writeByte(addr, A);
}

void CPU::lda()
{
    // grab the source address from the stream
    uint16_t addr = fetchWord();

    // load the accumulator from memory
    A = bus.readByte(addr);
}

void CPU::xchg()
{
    // swap the register pairs directly
    std::swap(H, D);
    std::swap(L, E);
}

void CPU::xthl()
{
    // exchange hl with the word at the stack pointer
    uint16_t stackVal = bus.readWord(SP);
    bus.writeWord(SP, HL);
    HL = stackVal;
}

void CPU::sphl()
{
    // direct load
    SP = HL;
}

void CPU::pchl()
{
    // jump to address in hl
    PC = HL;
}

void CPU::dadBC()
{
    // 16-bit addition
    uint32_t res = (uint32_t)HL + BC;

    // update carry flag (bit 16 of the result)
    statusFlags.Carry = (res > 0xFFFF) ? 1 : 0;

    // update hl
    HL = (uint16_t)(res & 0xFFFF);
}

void CPU::dadDE()
{
    // 16-bit addition
    uint32_t res = (uint32_t)HL + DE;

    // update carry flag (bit 16 of the result)
    statusFlags.Carry = (res > 0xFFFF) ? 1 : 0;

    // update hl
    HL = (uint16_t)(res & 0xFFFF);
}

void CPU::dadHL()
{
    // add HL to HL
    uint32_t res = (uint32_t)HL << 0x1;

    // update carry flag (bit 16 of the result)
    statusFlags.Carry = (res > 0xFFFF) ? 1 : 0;

    // update hl
    HL = (uint16_t)(res & 0xFFFF);
}

void CPU::dadSP()
{
    // 16-bit addition
    uint32_t res = (uint32_t)HL + SP;

    // update carry flag (bit 16 of the result)
    statusFlags.Carry = (res > 0xFFFF) ? 1 : 0;

    // update hl
    HL = (uint16_t)(res & 0xFFFF);
}

void CPU::staxBC()
{
    bus.writeByte(BC, A);
}

void CPU::staxDE()
{
    bus.writeByte(DE, A);
}

void CPU::ldaxBC()
{
    A = bus.readByte(BC);
}

void CPU::ldaxDE()
{
    A = bus.readByte(DE);
}

void CPU::movRegToReg()
{
    // mov register to register
    uint8_t& dest = getReg8((currentOpcode >> 3) & 0x7);
    uint8_t& src  = getReg8(currentOpcode & 0x7);
    dest = src;
}

void CPU::movRegToMem()
{
    // mov register to M (memory at HL)
    uint8_t& src = getReg8(currentOpcode & 0x7);
    bus.writeByte(HL, src);

}

void CPU::movMemToReg()
{
    // mov M to register
    uint8_t& dest = getReg8((currentOpcode >> 3) & 0x7);
    dest = bus.readByte(HL);
}

void CPU::movImmToReg()
{
    // move immediate to register
    // fetchByte() handles the PC increment here
    uint8_t& dest = getReg8((currentOpcode >> 3) & 0x7);
    dest = fetchByte();
}

void CPU::movImmToMem()
{
    // move immediate to M (memory at HL)
    // fetchByte() handles the PC increment here
    bus.writeByte(HL, fetchByte());
}

void CPU::inrR()
{
    uint8_t& reg = getReg8((currentOpcode >> 3) & 0x7);
    uint8_t before = reg;
    reg++;

    setZeroFlag(reg);
    setSignFlag(reg);
    setParityFlag(reg);
    setHalfCarryAdd(before, 1, reg);
}

void CPU::inrM()
{
    uint8_t val = bus.readByte(HL);
    uint8_t before = val;
    val++;
    bus.writeByte(HL, val);

    setZeroFlag(val);
    setSignFlag(val);
    setParityFlag(val);
    setHalfCarryAdd(before, 1, val);
}

void CPU::dcrR()
{
    uint8_t& reg = getReg8((currentOpcode >> 3) & 0x7);
    uint8_t before = reg;
    reg--;

    setZeroFlag(reg);
    setSignFlag(reg);
    setParityFlag(reg);
    setHalfCarrySub(before, 1, reg);
}

void CPU::dcrM()
{
    uint8_t val = bus.readByte(HL);
    uint8_t before = val;
    val--;
    bus.writeByte(HL, val);

    setZeroFlag(val);
    setSignFlag(val);
    setParityFlag(val);
    setHalfCarrySub(before, 1, val);
}

void CPU::addR() { alu_add(getReg8(currentOpcode & 0x7)); }
void CPU::adcR() { alu_adc(getReg8(currentOpcode & 0x7)); }
void CPU::subR() { alu_sub(getReg8(currentOpcode & 0x7)); }
void CPU::sbbR() { alu_sbb(getReg8(currentOpcode & 0x7)); }
void CPU::anaR() { alu_and(getReg8(currentOpcode & 0x7)); }
void CPU::xraR() { alu_xor(getReg8(currentOpcode & 0x7)); }
void CPU::oraR() { alu_or (getReg8(currentOpcode & 0x7)); }
void CPU::cmpR() { alu_cmp(getReg8(currentOpcode & 0x7)); }

void CPU::addM() { alu_add(bus.readByte(HL)); }
void CPU::adcM() { alu_adc(bus.readByte(HL)); }
void CPU::subM() { alu_sub(bus.readByte(HL)); }
void CPU::sbbM() { alu_sbb(bus.readByte(HL)); }
void CPU::anaM() { alu_and(bus.readByte(HL)); }
void CPU::xraM() { alu_xor(bus.readByte(HL)); }
void CPU::oraM() { alu_or (bus.readByte(HL)); }
void CPU::cmpM() { alu_cmp(bus.readByte(HL)); }

void CPU::adi() { alu_add(fetchByte()); }
void CPU::aci() { alu_adc(fetchByte()); }
void CPU::sui() { alu_sub(fetchByte()); }
void CPU::sbi() { alu_sbb(fetchByte()); }
void CPU::ani() { alu_and(fetchByte()); }
void CPU::xri() { alu_xor(fetchByte()); }
void CPU::ori() { alu_or (fetchByte()); }
void CPU::cpi() { alu_cmp(fetchByte()); }

void CPU::rlc()
{
    uint8_t cf = (A & 0x80) >> 7;
    statusFlags.Carry = cf;
    A = (A << 1) | cf;
}

void CPU::rrc()
{
    uint8_t cf = A & 0x1;
    statusFlags.Carry = cf;
    A = (A >> 1) | (cf << 7);
}

void CPU::ral()
{
    uint8_t old_cf = statusFlags.Carry;
    statusFlags.Carry = (A & 0x80) >> 7;
    A = (A << 1) | old_cf;
}

void CPU::rar()
{
    uint8_t old_cf = statusFlags.Carry;
    statusFlags.Carry = A & 0x1;
    A = (A >> 1) | (old_cf << 7);
}

void CPU::inxBC() { ++BC; }
void CPU::inxDE() { ++DE; }
void CPU::inxHL() { ++HL; }
void CPU::inxSP() { ++SP; }

void CPU::dcxBC() { --BC; }
void CPU::dcxDE() { --DE; }
void CPU::dcxHL() { --HL; }
void CPU::dcxSP() { --SP; }

void CPU::shld()
{
    // grab the address, then write the 16-bit value
    uint16_t addr = fetchWord();
    bus.writeWord(addr, HL);
}

void CPU::lhld()
{
    // grab the address, then read the 16-bit value
    uint16_t addr = fetchWord();
    HL = bus.readWord(addr);
}

void CPU::cma() { A = ~A; }
void CPU::stc() { statusFlags.Carry = 1; }
void CPU::cmc() { statusFlags.Carry = !statusFlags.Carry; }

void CPU::daa()
{
    uint8_t correction = 0;
    uint8_t carry = statusFlags.Carry;

    // decimal adjust accumulator logic
    if ((A & 0x0F) > 9 || statusFlags.AuxCarry)
    {
        correction |= 0x06;
    }
    if ((A >> 4) > 9 || ((A >> 4) >= 9 && (A & 0x0F) > 9) || carry)
    {
        correction |= 0x60;
        carry = 1;
    }

    alu_add(correction);
    statusFlags.Carry = carry;
}

void CPU::rst()
{
    // extract vector address from bits 3-5 of the opcode
    uint16_t rst_address = (currentOpcode & 0b00111000);
    pushWord(PC); // PC is already at the next instruction after fetch
    PC = rst_address;
}

void CPU::ei()  { systemFlags.interruptEnabled = true; }
void CPU::di()  { systemFlags.interruptEnabled = false; }
void CPU::hlt() { systemFlags.halted = true; }

void CPU::nop() { /* do nothing */ }


const CPU::Instruction CPU::OPCODE_TABLE[256] =
{
    // [Opcode] = { "Mnemonic", Cycles, &CPU::HandlerMethod }

    // 0x00
    [0x00] = { "NOP",        4,  &CPU::nop },
    [0x01] = { "LXI B,D16", 10,  &CPU::lxiBC },
    [0x02] = { "STAX B",     7,  &CPU::staxBC },
    [0x03] = { "INX B",      5,  &CPU::inxBC },
    [0x04] = { "INR B",      5,  &CPU::inrR },
    [0x05] = { "DCR B",      5,  &CPU::dcrR },
    [0x06] = { "MVI B,D8",   7,  &CPU::movImmToReg },
    [0x07] = { "RLC",        4,  &CPU::rlc },
    [0x08] = { "NOP",        4,  &CPU::nop },
    [0x09] = { "DAD B",     10,  &CPU::dadBC },
    [0x0A] = { "LDAX B",     7,  &CPU::ldaxBC },
    [0x0B] = { "DCX B",      5,  &CPU::dcxBC },
    [0x0C] = { "INR C",      5,  &CPU::inrR },
    [0x0D] = { "DCR C",      5,  &CPU::dcrR },
    [0x0E] = { "MVI C,D8",   7,  &CPU::movImmToReg },
    [0x0F] = { "RRC",        4,  &CPU::rrc },

    // 0x10
    [0x10] = { "NOP",    4,  &CPU::nop },
    [0x11] = { "LXI D", 10,  &CPU::lxiDE },
    [0x12] = { "STAX D", 7,  &CPU::staxDE },
    [0x13] = { "INX D",  5,  &CPU::inxDE },
    [0x14] = { "INR D",  5,  &CPU::inrR },
    [0x15] = { "DCR D",  5,  &CPU::dcrR },
    [0x16] = { "MVI D",  7,  &CPU::movImmToReg },
    [0x17] = { "RAL",    4,  &CPU::ral },
    [0x18] = { "NOP",    4,  &CPU::nop },
    [0x19] = { "DAD D", 10,  &CPU::dadDE },
    [0x1A] = { "LDAX D", 7,  &CPU::ldaxDE },
    [0x1B] = { "DCX D",  5,  &CPU::dcxDE },
    [0x1C] = { "INR E",  5,  &CPU::inrR },
    [0x1D] = { "DCR E",  5,  &CPU::dcrR },
    [0x1E] = { "MVI E",  7,  &CPU::movImmToReg },
    [0x1F] = { "RAR",    4,  &CPU::rar },

    // 0x20
    [0x20] = { "NOP",    4,  &CPU::nop },
    [0x21] = { "LXI H", 10,  &CPU::lxiHL },
    [0x22] = { "SHLD",  16,  &CPU::shld },
    [0x23] = { "INX H",  5,  &CPU::inxHL },
    [0x24] = { "INR H",  5,  &CPU::inrR },
    [0x25] = { "DCR H",  5,  &CPU::dcrR },
    [0x26] = { "MVI H",  7,  &CPU::movImmToReg },
    [0x27] = { "DAA",    4,  &CPU::daa },
    [0x28] = { "NOP",    4,  &CPU::nop },
    [0x29] = { "DAD H", 10,  &CPU::dadHL },
    [0x2A] = { "LHLD",  16,  &CPU::lhld },
    [0x2B] = { "DCX H",  5,  &CPU::dcxHL },
    [0x2C] = { "INR L",  5,  &CPU::inrR },
    [0x2D] = { "DCR L",  5,  &CPU::dcrR },
    [0x2E] = { "MVI L",  7,  &CPU::movImmToReg },
    [0x2F] = { "CMA",    4,  &CPU::cma },

    // 0x30
    [0x30] = { "NOP",     4,  &CPU::nop },
    [0x31] = { "LXI SP", 10,  &CPU::lxiSP },
    [0x32] = { "STA",    13,  &CPU::sta },
    [0x33] = { "INX SP",  5,  &CPU::inxSP },
    [0x34] = { "INR M",  10,  &CPU::inrM },
    [0x35] = { "DCR M",  10,  &CPU::dcrM },
    [0x36] = { "MVI M",  10,  &CPU::movImmToMem },
    [0x37] = { "STC",     4,  &CPU::stc },
    [0x38] = { "NOP",     4,  &CPU::nop },
    [0x39] = { "DAD SP", 10,  &CPU::dadSP },
    [0x3A] = { "LDA",    13,  &CPU::lda },
    [0x3B] = { "DCX SP",  5,  &CPU::dcxSP },
    [0x3C] = { "INR A",   5,  &CPU::inrR },
    [0x3D] = { "DCR A",   5,  &CPU::dcrR },
    [0x3E] = { "MVI A",   7,  &CPU::movImmToReg },
    [0x3F] = { "CMC",     4,  &CPU::cmc },

    // 0x40
    [0x40] = { "MOV B,B", 5, &CPU::movRegToReg },
    [0x41] = { "MOV B,C", 5, &CPU::movRegToReg },
    [0x42] = { "MOV B,D", 5, &CPU::movRegToReg },
    [0x43] = { "MOV B,E", 5, &CPU::movRegToReg },
    [0x44] = { "MOV B,H", 5, &CPU::movRegToReg },
    [0x45] = { "MOV B,L", 5, &CPU::movRegToReg },
    [0x46] = { "MOV B,M", 7, &CPU::movMemToReg },
    [0x47] = { "MOV B,A", 5, &CPU::movRegToReg },
    [0x48] = { "MOV C,B", 5, &CPU::movRegToReg },
    [0x49] = { "MOV C,C", 5, &CPU::movRegToReg },
    [0x4A] = { "MOV C,D", 5, &CPU::movRegToReg },
    [0x4B] = { "MOV C,E", 5, &CPU::movRegToReg },
    [0x4C] = { "MOV C,H", 5, &CPU::movRegToReg },
    [0x4D] = { "MOV C,L", 5, &CPU::movRegToReg },
    [0x4E] = { "MOV C,M", 7, &CPU::movMemToReg },
    [0x4F] = { "MOV C,A", 5, &CPU::movRegToReg },

    // 0x50
    // [0x50 - 0x57] MOV D, R/M
    [0x50] = { "MOV D,B", 5, &CPU::movRegToReg },
    [0x51] = { "MOV D,C", 5, &CPU::movRegToReg },
    [0x52] = { "MOV D,D", 5, &CPU::movRegToReg },
    [0x53] = { "MOV D,E", 5, &CPU::movRegToReg },
    [0x54] = { "MOV D,H", 5, &CPU::movRegToReg },
    [0x55] = { "MOV D,L", 5, &CPU::movRegToReg },
    [0x56] = { "MOV D,M", 7, &CPU::movMemToReg },
    [0x57] = { "MOV D,A", 5, &CPU::movRegToReg },
    [0x58] = { "MOV E,B", 5, &CPU::movRegToReg },
    [0x59] = { "MOV E,C", 5, &CPU::movRegToReg },
    [0x5A] = { "MOV E,D", 5, &CPU::movRegToReg },
    [0x5B] = { "MOV E,E", 5, &CPU::movRegToReg },
    [0x5C] = { "MOV E,H", 5, &CPU::movRegToReg },
    [0x5D] = { "MOV E,L", 5, &CPU::movRegToReg },
    [0x5E] = { "MOV E,M", 7, &CPU::movMemToReg },
    [0x5F] = { "MOV E,A", 5, &CPU::movRegToReg },

    // 0x60
    [0x60] = { "MOV H,B", 5,  &CPU::movRegToReg },
    [0x61] = { "MOV H,C", 5,  &CPU::movRegToReg },
    [0x62] = { "MOV H,D", 5,  &CPU::movRegToReg },
    [0x63] = { "MOV H,E", 5,  &CPU::movRegToReg },
    [0x64] = { "MOV H,H", 5,  &CPU::movRegToReg },
    [0x65] = { "MOV H,L", 5,  &CPU::movRegToReg },
    [0x66] = { "MOV H,M", 7,  &CPU::movMemToReg },
    [0x67] = { "MOV H,A", 5,  &CPU::movRegToReg },
    [0x68] = { "MOV L,B", 5,  &CPU::movRegToReg },
    [0x69] = { "MOV L,C", 5,  &CPU::movRegToReg },
    [0x6A] = { "MOV L,D", 5,  &CPU::movRegToReg },
    [0x6B] = { "MOV L,E", 5,  &CPU::movRegToReg },
    [0x6C] = { "MOV L,H", 5,  &CPU::movRegToReg },
    [0x6D] = { "MOV L,L", 5,  &CPU::movRegToReg },
    [0x6E] = { "MOV L,M", 7,  &CPU::movMemToReg },
    [0x6F] = { "MOV L,A", 5,  &CPU::movRegToReg },

    // 0x70
    [0x70] = { "MOV M,B", 7, &CPU::movRegToMem },
    [0x71] = { "MOV M,C", 7, &CPU::movRegToMem },
    [0x72] = { "MOV M,D", 7, &CPU::movRegToMem },
    [0x73] = { "MOV M,E", 7, &CPU::movRegToMem },
    [0x74] = { "MOV M,H", 7, &CPU::movRegToMem },
    [0x75] = { "MOV M,L", 7, &CPU::movRegToMem },
    [0x76] = { "HLT",     5, &CPU::hlt },
    [0x77] = { "MOV M,A", 7, &CPU::movRegToMem },
    [0x78] = { "MOV A,B", 5, &CPU::movRegToReg },
    [0x79] = { "MOV A,C", 5, &CPU::movRegToReg },
    [0x7A] = { "MOV A,D", 5, &CPU::movRegToReg },
    [0x7B] = { "MOV A,E", 5, &CPU::movRegToReg },
    [0x7C] = { "MOV A,H", 5, &CPU::movRegToReg },
    [0x7D] = { "MOV A,L", 5, &CPU::movRegToReg },
    [0x7E] = { "MOV A,M", 7, &CPU::movMemToReg },
    [0x7F] = { "MOV A,A", 5, &CPU::movRegToReg },

    // 0x80
    [0x80] = { "ADD B", 4, &CPU::addR },
    [0x81] = { "ADD C", 4, &CPU::addR },
    [0x82] = { "ADD D", 4, &CPU::addR },
    [0x83] = { "ADD E", 4, &CPU::addR },
    [0x84] = { "ADD H", 4, &CPU::addR },
    [0x85] = { "ADD L", 4, &CPU::addR },
    [0x86] = { "ADD M", 7, &CPU::addM },
    [0x87] = { "ADD A", 4, &CPU::addR },
    [0x88] = { "ADC B", 4, &CPU::adcR },
    [0x89] = { "ADC C", 4, &CPU::adcR },
    [0x8A] = { "ADC D", 4, &CPU::adcR },
    [0x8B] = { "ADC E", 4, &CPU::adcR },
    [0x8C] = { "ADC H", 4, &CPU::adcR },
    [0x8D] = { "ADC L", 4, &CPU::adcR },
    [0x8E] = { "ADC M", 7, &CPU::adcM },
    [0x8F] = { "ADC A", 4, &CPU::adcR },

    // 0x90
    [0x90] = { "SUB B", 4, &CPU::subR },
    [0x91] = { "SUB C", 4, &CPU::subR },
    [0x92] = { "SUB D", 4, &CPU::subR },
    [0x93] = { "SUB E", 4, &CPU::subR },
    [0x94] = { "SUB H", 4, &CPU::subR },
    [0x95] = { "SUB L", 4, &CPU::subR },
    [0x96] = { "SUB M", 7, &CPU::subM },
    [0x97] = { "SUB A", 4, &CPU::subR },
    [0x98] = { "SBB B", 4, &CPU::sbbR },
    [0x99] = { "SBB C", 4, &CPU::sbbR },
    [0x9A] = { "SBB D", 4, &CPU::sbbR },
    [0x9B] = { "SBB E", 4, &CPU::sbbR },
    [0x9C] = { "SBB H", 4, &CPU::sbbR },
    [0x9D] = { "SBB L", 4, &CPU::sbbR },
    [0x9E] = { "SBB M", 7, &CPU::sbbM },
    [0x9F] = { "SBB A", 4, &CPU::sbbR },

    // 0xA0
    [0xA0] = { "ANA B",       4,  &CPU::anaR },
    [0xA1] = { "ANA C",       4,  &CPU::anaR },
    [0xA2] = { "ANA D",       4,  &CPU::anaR },
    [0xA3] = { "ANA E",       4,  &CPU::anaR },
    [0xA4] = { "ANA H",       4,  &CPU::anaR },
    [0xA5] = { "ANA L",       4,  &CPU::anaR },
    [0xA6] = { "ANA M",       7,  &CPU::anaM },
    [0xA7] = { "ANA A",       4,  &CPU::anaR },
    [0xA8] = { "XRA B",       4,  &CPU::xraR },
    [0xA9] = { "XRA C",       4,  &CPU::xraR },
    [0xAA] = { "XRA D",       4,  &CPU::xraR },
    [0xAB] = { "XRA E",       4,  &CPU::xraR },
    [0xAC] = { "XRA H",       4,  &CPU::xraR },
    [0xAD] = { "XRA L",       4,  &CPU::xraR },
    [0xAE] = { "XRA M",       7,  &CPU::xraM },
    [0xAF] = { "XRA A",       4,  &CPU::xraR },

    // 0xB0
    [0xB0] = { "ORA B",       4,  &CPU::oraR },
    [0xB1] = { "ORA C",       4,  &CPU::oraR },
    [0xB2] = { "ORA D",       4,  &CPU::oraR },
    [0xB3] = { "ORA E",       4,  &CPU::oraR },
    [0xB4] = { "ORA H",       4,  &CPU::oraR },
    [0xB5] = { "ORA L",       4,  &CPU::oraR },
    [0xB6] = { "ORA M",       7,  &CPU::oraM },
    [0xB7] = { "ORA A",       4,  &CPU::oraR },
    [0xB8] = { "CMP B",       4,  &CPU::cmpR },
    [0xB9] = { "CMP C",       4,  &CPU::cmpR },
    [0xBA] = { "CMP D",       4,  &CPU::cmpR },
    [0xBB] = { "CMP E",       4,  &CPU::cmpR },
    [0xBC] = { "CMP H",       4,  &CPU::cmpR },
    [0xBD] = { "CMP L",       4,  &CPU::cmpR },
    [0xBE] = { "CMP M",       7,  &CPU::cmpM },
    [0xBF] = { "CMP A",       4,  &CPU::cmpR },

    // 0xC0
    [0xC0] = { "RNZ",      5,  &CPU::rnz },
    [0xC1] = { "POP B",   10,  &CPU::popBC },
    [0xC2] = { "JNZ",     10,  &CPU::jnz },
    [0xC3] = { "JMP",     10,  &CPU::jmp },
    [0xC4] = { "CNZ",     11,  &CPU::cnz },
    [0xC5] = { "PUSH B",  11,  &CPU::pushBC },
    [0xC6] = { "ADI",      7,  &CPU::adi },
    [0xC7] = { "RST 0",   11,  &CPU::rst },
    [0xC8] = { "RZ",       5,  &CPU::rz },
    [0xC9] = { "RET",     10,  &CPU::ret },
    [0xCA] = { "JZ",      10,  &CPU::jz },
    [0xCB] = { "JMP",     10,  &CPU::jmp },
    [0xCC] = { "CZ",      11,  &CPU::cz },
    [0xCD] = { "CALL",    17,  &CPU::call },
    [0xCE] = { "ACI",      7,  &CPU::aci },
    [0xCF] = { "RST 1",   11,  &CPU::rst },

    // 0xD0
    [0xD0] = { "RNC",         5,  &CPU::rnc },
    [0xD1] = { "POP D",      10,  &CPU::popDE },
    [0xD2] = { "JNC A16",    10,  &CPU::jnc },
    [0xD3] = { "OUT D8",     10,  &CPU::out },
    [0xD4] = { "CNC A16",    11,  &CPU::cnc },
    [0xD5] = { "PUSH D",     11,  &CPU::pushDE },
    [0xD6] = { "SUI D8",      7,  &CPU::sui },
    [0xD7] = { "RST 2",      11,  &CPU::rst },
    [0xD8] = { "RC",          5,  &CPU::rc },
    [0xD9] = { "RET",        10,  &CPU::ret },
    [0xDA] = { "JC A16",     10,  &CPU::jc },
    [0xDB] = { "IN D8",      10,  &CPU::in },
    [0xDC] = { "CC A16",     11,  &CPU::cc },
    [0xDD] = { "CALL A16",   17,  &CPU::call },
    [0xDE] = { "SBI D8",      7,  &CPU::sbi },
    [0xDF] = { "RST 3",      11,  &CPU::rst },

    // 0xE0
    [0xE0] = { "RPO",         5,  &CPU::rpo },
    [0xE1] = { "POP H",      10,  &CPU::popHL },
    [0xE2] = { "JPO A16",    10,  &CPU::jpo },
    [0xE3] = { "XTHL",       18,  &CPU::xthl },
    [0xE4] = { "CPO A16",    11,  &CPU::cpo },
    [0xE5] = { "PUSH H",     11,  &CPU::pushHL },
    [0xE6] = { "ANI D8",      7,  &CPU::ani },
    [0xE7] = { "RST 4",      11,  &CPU::rst },
    [0xE8] = { "RPE",         5,  &CPU::rpe },
    [0xE9] = { "PCHL",        5,  &CPU::pchl },
    [0xEA] = { "JPE A16",    10,  &CPU::jpe },
    [0xEB] = { "XCHG",        4,  &CPU::xchg },
    [0xEC] = { "CPE A16",    11,  &CPU::cpe },
    [0xED] = { "CALL A16",   17,  &CPU::call },
    [0xEE] = { "XRI D8",      7,  &CPU::xri },
    [0xEF] = { "RST 5",      11,  &CPU::rst },

    // 0xF0
    [0xF0] = { "RP",          5,  &CPU::rp },
    [0xF1] = { "POP PSW",    10,  &CPU::popPSW },
    [0xF2] = { "JP A16",     10,  &CPU::jp },
    [0xF3] = { "DI",          4,  &CPU::di },
    [0xF4] = { "CP A16",     11,  &CPU::cp },
    [0xF5] = { "PUSH PSW",   11,  &CPU::pushPSW },
    [0xF6] = { "ORI D8",      7,  &CPU::ori },
    [0xF7] = { "RST 6",      11,  &CPU::rst },
    [0xF8] = { "RM",          5,  &CPU::rm },
    [0xF9] = { "SPHL",        5,  &CPU::sphl },
    [0xFA] = { "JM A16",     10,  &CPU::jm },
    [0xFB] = { "EI",          4,  &CPU::ei },
    [0xFC] = { "CM A16",     11,  &CPU::cm },
    [0xFD] = { "CALL A16",   17,  &CPU::call },
    [0xFE] = { "CPI D8",      7,  &CPU::cpi },
    [0xFF] = { "RST 7",      11,  &CPU::rst }
};
