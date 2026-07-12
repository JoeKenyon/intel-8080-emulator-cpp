#include <cstddef>
#include <utility>
#include "CPU.h"

CPU::CPU(IBus& b) : m_bus(b)
{
    PC = 0;
    SP = 0;
    A = 0; B = 0; C = 0; D = 0; E = 0; H = 0; L = 0;

    // clear all flags
    statusFlags.packed = 0;
    statusFlags._one = 1;
    statusFlags._zero1 = 0;
    statusFlags._zero2 = 0;

    // disable interrupts
    systemFlags.interruptEnabled = false;
    systemFlags.halted = false;
    m_extraCycles = 0;
}

bool CPU::evaluateCondition(uint8_t condCode)
{
    switch (condCode)
    {
        case 0: return !statusFlags.Zero;    // nz (not zero)
        case 1: return statusFlags.Zero;     // z  (zero)
        case 2: return !statusFlags.Carry;   // nc (no carry)
        case 3: return statusFlags.Carry;    // c  (carry)
        case 4: return !statusFlags.Parity;  // po (parity odd)
        case 5: return statusFlags.Parity;   // pe (parity even)
        case 6: return !statusFlags.Sign;    // p  (positive)
        case 7: return statusFlags.Sign;     // m  (minus)
        default: return false;
    }
}

void CPU::jump()
{
    uint16_t addr = fetchWord();

    // 0xc3 is the only unconditional jump in this cluster
    bool isConditional = (m_currentOpcode != 0xC3);
    uint8_t conditionCode = (m_currentOpcode >> 3) & 0x07;

    if (!isConditional || evaluateCondition(conditionCode))
    {
        PC = addr;
    }
}

void CPU::call()
{
    uint16_t addr = fetchWord();

    // 0xcd is the unconditional call
    bool isConditional = (m_currentOpcode != 0xCD);
    uint8_t conditionCode = (m_currentOpcode >> 3) & 0x07;

    if (!isConditional || evaluateCondition(conditionCode))
    {
        pushWord(PC);
        PC = addr;

        // branch taken costs extra
        if (isConditional) m_extraCycles = 6;
    }
}

void CPU::ret()
{
    // 0xc9 is the unconditional return
    bool isConditional = (m_currentOpcode != 0xC9);
    uint8_t conditionCode = (m_currentOpcode >> 3) & 0x07;

    if (!isConditional || evaluateCondition(conditionCode))
    {
        PC = popWord();
        if (isConditional) m_extraCycles = 6;
    }
}

// handles all push operations
void CPU::push()
{
    // bits 4 and 5 determine the target register pair
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;
    pushWord(getReg16(pairCode, true));
}

// handles all pop operations
void CPU::pop()
{
    // pop the word first, then route it to the right pair
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;
    uint16_t val = popWord();
    setReg16(pairCode, val, true);
}

void CPU::lxi()
{
    // grab bits 4 and 5 to figure out where this word is going
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;
    uint16_t val = fetchWord();
    setReg16(pairCode, val);
}

// handles sta, lda, shld, and lhld
void CPU::directStoreLoad()
{
    // all four instructions fetch a 16-bit memory address first
    uint16_t addr = fetchWord();

    // bit 3 tells us if we are reading (load) or writing (store)
    bool isLoad = (m_currentOpcode & 0x08) != 0;

    // bit 4 tells us if we are targeting the accumulator or the hl pair
    bool isAccumulator = (m_currentOpcode & 0x10) != 0;

    if (isAccumulator)
    {
        if (isLoad) A = m_bus.readByte(addr);   // lda
        else m_bus.writeByte(addr, A);   // sta
    }
    else
    {
        if (isLoad) HL = m_bus.readWord(addr); // lhld
        else m_bus.writeWord(addr, HL); // shld
    }
}

// handles stax b, stax d, ldax b, ldax d
void CPU::indirectStoreLoad()
{
    // grab the register pair code from bits 4 and 5
    // 0 = bc, 1 = de
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;

    // bit 3 tells us if we are reading (load) or writing (store)
    bool isLoad = (m_currentOpcode & 0x08) != 0;

    // fetch the memory address from that register pair
    uint16_t addr = getReg16(pairCode);

    // ldax: load memory into accumulator
    if (isLoad) A = m_bus.readByte(addr);
    // stax: store accumulator into memory
    else m_bus.writeByte(addr, A);
}

void CPU::dad()
{
    // extract pair code (0=bc, 1=de, 2=hl, 3=sp)
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;
    uint16_t regPair = getReg16(pairCode);

    // 16-bit addition
    uint32_t res = static_cast<uint32_t>(HL) + regPair;

    // update carry flag (bit 16 of the result)
    statusFlags.Carry = (res > 0xFFFF) ? 1 : 0;

    // update hl
    HL = static_cast<uint16_t>(res & 0xffff);
}

void CPU::inr()
{
    // extract target register code from bits 3-5
    uint8_t regCode = (m_currentOpcode >> 3) & 0x07;
    uint8_t oldVal = getReg8(regCode);
    uint8_t newVal = oldVal + 1;

    // set flags based on the increment
    setHalfCarryAdd(oldVal, 1, newVal);
    setZSP(newVal);

    setReg8(regCode, newVal);
}

void CPU::dcr()
{
    // extract target register code from bits 3-5
    uint8_t regCode = (m_currentOpcode >> 3) & 0x07;
    uint8_t oldVal = getReg8(regCode);
    uint8_t newVal = oldVal - 1;

    // set flags based on the decrement
    setHalfCarrySub(oldVal, 1, newVal);
    setZeroFlag(newVal);
    setSignFlag(newVal);
    setParityFlag(newVal);

    setReg8(regCode, newVal);
}

void CPU::mvi()
{
    // extract destination (bits 3-5)
    uint8_t destCode = (m_currentOpcode >> 3) & 0x07;

    // grab and write the immediate value from the instruction stream
    setReg8(destCode, fetchByte());
}

void CPU::mov()
{
    // extract destination (bits 3-5) and source (bits 0-2)
    uint8_t destCode = (m_currentOpcode >> 3) & 0x07;
    uint8_t srcCode  = m_currentOpcode & 0x07;
    setReg8(destCode, getReg8(srcCode));
}

void CPU::inx()
{
    // grab the register pair code from bits 4 and 5
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;

    // read the pair (defaults to SP for pair 3)
    // increment and write back
    setReg16(pairCode, getReg16(pairCode) + 1);
}

void CPU::dcx()
{
    // grab the register pair code from bits 4 and 5
    uint8_t pairCode = (m_currentOpcode >> 4) & 0x03;

    // read the pair (defaults to SP for pair 3)
    // decrement and write back
    setReg16(pairCode, getReg16(pairCode) - 1);
}

void CPU::alu()
{
    // bits 0-2 tell us where the data is coming from
    uint8_t srcCode = m_currentOpcode & 0x07;
    uint8_t operand = 0;

    // get the operand
    operand = getReg8(srcCode);

    // bits 3-5 tell us exactly which math operation to run
    uint8_t opType = (m_currentOpcode >> 3) & 0x07;

    // hand off to shared helper
    (this->*aluOps[opType])(operand);
}

void CPU::aluImmediate()
{
    // grab the byte from the program stream
    uint8_t operand = fetchByte();

    // extracts math operation from bits 3-5
    uint8_t opType = (m_currentOpcode >> 3) & 0x07;

    // hand off to shared helper
    (this->*aluOps[opType])(operand);
}

void CPU::requestInterrupt(uint8_t vector)
{
    // if the program called di() to disable interrupts, ignore the hardware
    if (!systemFlags.interruptEnabled)
    {
        return;
    }

    // the moment an interrupt fires, the cpu automatically acts like di() was called.
    // the game must manually call ei() before it can receive the next half-frame interrupt.
    systemFlags.interruptEnabled = false;

    // if the cpu was sleeping via hlt(), the interrupt wakes it back up
    systemFlags.halted = false;

    // perform the exact same logic as  software rst() handler
    pushWord(PC);
    PC = vector;
}

uint8_t CPU::getReg8(uint8_t code)
{
    switch (code)
    {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 6: return m_bus.readByte(HL);
        case 7: return A;
    }
    return 0xFF; // unreachable, silences warning
}

void CPU::setReg8(uint8_t code, uint8_t val)
{
    switch (code)
    {
        case 0: B = val; return;
        case 1: C = val; return;
        case 2: D = val; return;
        case 3: E = val; return;
        case 4: H = val; return;
        case 5: L = val; return;
        case 6: m_bus.writeByte(HL, val); return;
        case 7: A = val; return;
    }
}

uint16_t CPU::getReg16(uint8_t code, bool stackOp)
{
    switch (code)
    {
        case 0: return BC;
        case 1: return DE;
        case 2: return HL;
        case 3: return stackOp ? static_cast<uint16_t>(PSW) : SP;
    }
    return 0xFFFF; // unreachable
}

void CPU::setReg16(uint8_t code, uint16_t val, bool stackOp)
{
    switch (code)
    {
        case 0: BC = val; return;
        case 1: DE = val; return;
        case 2: HL = val; return;
        case 3:
            if (stackOp)
            {
                PSW = val;
                statusFlags._one   = 1;
                statusFlags._zero1 = 0;
                statusFlags._zero2 = 0;
            }
            else
            {
                SP = val;
            }
            return;
    }
}

// Logic Operations
void CPU::alu_and(uint8_t val)
{
    statusFlags.AuxCarry = ((A | val) & 0x08) != 0;
    statusFlags.Carry = 0;
    A &= val;
    setZSP(A);
}

void CPU::alu_xor(uint8_t val)
{
    statusFlags.AuxCarry = 0;
    statusFlags.Carry = 0;
    A ^= val;
    setZSP(A);
}

void CPU::alu_or(uint8_t val)
{
    statusFlags.AuxCarry = 0;
    statusFlags.Carry = 0;
    A |= val;
    setZSP(A);
}

// Arithmetic Operations
void CPU::alu_add(uint8_t val)
{
    uint16_t tmp = A + val;
    setHalfCarryAdd(A, val, tmp);
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZSP(A);
}

void CPU::alu_adc(uint8_t val)
{
    uint16_t tmp = A + val + statusFlags.Carry;
    setHalfCarryAdd(A, val, tmp);
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZSP(A);
}

void CPU::alu_sub(uint8_t val)
{
    uint16_t tmp = A - val;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp); // Note: 8080 Carry flag is set on borrow
    A = (uint8_t)(tmp & 0xFF);
    setZSP(A);
}

void CPU::alu_sbb(uint8_t val)
{
    uint16_t tmp = A - val - statusFlags.Carry;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp);
    A = (uint8_t)(tmp & 0xFF);
    setZSP(A);
}

void CPU::alu_cmp(uint8_t val)
{
    uint16_t tmp = A - val;
    setHalfCarrySub(A, val, (uint8_t)(tmp & 0xFF));
    setCarryFlag(tmp);
    setZSP((uint8_t)(tmp & 0xFF));
}

void CPU::pushByte(uint8_t value)
{
    m_bus.writeByte(--SP, value);
}

uint8_t CPU::popByte()
{
    return m_bus.readByte(SP++);
}

void CPU::pushWord(uint16_t value)
{
    // 8080 pushes the high byte first, then the low byte
    m_bus.writeByte(--SP, (value >> 8) & 0xFF);
    m_bus.writeByte(--SP, value & 0xFF);
}

uint16_t CPU::popWord()
{
    // 8080 pops the low byte first, then the high byte
    uint16_t low  = m_bus.readByte(SP++);
    uint16_t high = m_bus.readByte(SP++);
    return (high << 8) | low;
}

void CPU::in()
{
    // grab the port number from the next byte
    uint8_t port = fetchByte();

    // read from the port via the bus
    A = m_bus.readPort(port);
}

void CPU::out()
{
    // grab the port number from the next byte
    uint8_t port = fetchByte();

    // write the accumulator to the port via the bus
    m_bus.writePort(port, A);
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
    uint16_t stackVal = m_bus.readWord(SP);
    m_bus.writeWord(SP, HL);
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

void CPU::rotate()
{
    // extract rotate type from bits 3 and 4
    uint8_t rotateType = (m_currentOpcode >> 3) & 0x03;

    switch (rotateType)
    {
        case 0: // rlc (0x07)
        {
            uint8_t cf = (A & 0x80) >> 7;
            statusFlags.Carry = cf;
            A = (A << 1) | cf;
            break;
        }
        case 1: // rrc (0x0F)
        {
            uint8_t cf = A & 0x01;
            statusFlags.Carry = cf;
            A = (A >> 1) | (cf << 7);
            break;
        }
        case 2: // ral (0x17)
        {
            uint8_t old_cf = statusFlags.Carry;
            statusFlags.Carry = (A & 0x80) >> 7;
            A = (A << 1) | old_cf;
            break;
        }
        case 3: // rar (0x1F)
        {
            uint8_t old_cf = statusFlags.Carry;
            statusFlags.Carry = A & 0x01;
            A = (A >> 1) | (old_cf << 7);
            break;
        }
    }
}

void CPU::cma()
{
    A = ~A;
}

void CPU::stc()
{
    statusFlags.Carry = 1;
}

void CPU::cmc()
{
    statusFlags.Carry = !statusFlags.Carry;
}

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
    uint16_t rst_address = (m_currentOpcode & 0b00111000);
    pushWord(PC); // PC is already at the next instruction after fetch
    PC = rst_address;
}

void CPU::ei()
{
    systemFlags.interruptEnabled = true;
}

void CPU::di()
{
    systemFlags.interruptEnabled = false;
}

void CPU::hlt()
{
    systemFlags.halted = true;
}

void CPU::nop()
{
    return;
}

const CPU::Instruction CPU::OPCODE_TABLE[256] =
{
    {"NOP", 4, &CPU::nop}, {"LXI B,d16", 10, &CPU::lxi}, {"STAX B", 7, &CPU::indirectStoreLoad}, {"INX B", 5, &CPU::inx}, {"INR B", 5, &CPU::inr}, {"DCR B", 5, &CPU::dcr}, {"MVI B,d8", 7, &CPU::mvi}, {"RLC", 4, &CPU::rotate}, {"NOP", 4, &CPU::nop}, {"DAD B", 10, &CPU::dad}, {"LDAX B", 7, &CPU::indirectStoreLoad}, {"DCX B", 5, &CPU::dcx}, {"INR C", 5, &CPU::inr}, {"DCR C", 5, &CPU::dcr}, {"MVI C,d8", 7, &CPU::mvi}, {"RRC", 4, &CPU::rotate},  // 0x00-0x0F
    {"NOP", 4, &CPU::nop}, {"LXI D,d16", 10, &CPU::lxi}, {"STAX D", 7, &CPU::indirectStoreLoad}, {"INX D", 5, &CPU::inx}, {"INR D", 5, &CPU::inr}, {"DCR D", 5, &CPU::dcr}, {"MVI D,d8", 7, &CPU::mvi}, {"RAL", 4, &CPU::rotate}, {"NOP", 4, &CPU::nop}, {"DAD D", 10, &CPU::dad}, {"LDAX D", 7, &CPU::indirectStoreLoad}, {"DCX D", 5, &CPU::dcx}, {"INR E", 5, &CPU::inr}, {"DCR E", 5, &CPU::dcr}, {"MVI E,d8", 7, &CPU::mvi}, {"RAR", 4, &CPU::rotate},  // 0x10-0x1F
    {"NOP", 4, &CPU::nop}, {"LXI H,d16", 10, &CPU::lxi}, {"SHLD a16", 16, &CPU::directStoreLoad}, {"INX H", 5, &CPU::inx}, {"INR H", 5, &CPU::inr}, {"DCR H", 5, &CPU::dcr}, {"MVI H,d8", 7, &CPU::mvi}, {"DAA", 4, &CPU::daa}, {"NOP", 4, &CPU::nop}, {"DAD H", 10, &CPU::dad}, {"LHLD a16", 16, &CPU::directStoreLoad}, {"DCX H", 5, &CPU::dcx}, {"INR L", 5, &CPU::inr}, {"DCR L", 5, &CPU::dcr}, {"MVI L,d8", 7, &CPU::mvi}, {"CMA", 4, &CPU::cma},  // 0x20-0x2F
    {"NOP", 4, &CPU::nop}, {"LXI SP,d16", 10, &CPU::lxi}, {"STA a16", 13, &CPU::directStoreLoad}, {"INX SP", 5, &CPU::inx}, {"INR M", 10, &CPU::inr}, {"DCR M", 10, &CPU::dcr}, {"MVI M,d8", 10, &CPU::mvi}, {"STC", 4, &CPU::stc}, {"NOP", 4, &CPU::nop}, {"DAD SP", 10, &CPU::dad}, {"LDA a16", 13, &CPU::directStoreLoad}, {"DCX SP", 5, &CPU::dcx}, {"INR A", 5, &CPU::inr}, {"DCR A", 5, &CPU::dcr}, {"MVI A,d8", 7, &CPU::mvi}, {"CMC", 4, &CPU::cmc},  // 0x30-0x3F
    {"MOV B,B", 5, &CPU::mov}, {"MOV B,C", 5, &CPU::mov}, {"MOV B,D", 5, &CPU::mov}, {"MOV B,E", 5, &CPU::mov}, {"MOV B,H", 5, &CPU::mov}, {"MOV B,L", 5, &CPU::mov}, {"MOV B,M", 7, &CPU::mov}, {"MOV B,A", 5, &CPU::mov}, {"MOV C,B", 5, &CPU::mov}, {"MOV C,C", 5, &CPU::mov}, {"MOV C,D", 5, &CPU::mov}, {"MOV C,E", 5, &CPU::mov}, {"MOV C,H", 5, &CPU::mov}, {"MOV C,L", 5, &CPU::mov}, {"MOV C,M", 7, &CPU::mov}, {"MOV C,A", 5, &CPU::mov},  // 0x40-0x4F
    {"MOV D,B", 5, &CPU::mov}, {"MOV D,C", 5, &CPU::mov}, {"MOV D,D", 5, &CPU::mov}, {"MOV D,E", 5, &CPU::mov}, {"MOV D,H", 5, &CPU::mov}, {"MOV D,L", 5, &CPU::mov}, {"MOV D,M", 7, &CPU::mov}, {"MOV D,A", 5, &CPU::mov}, {"MOV E,B", 5, &CPU::mov}, {"MOV E,C", 5, &CPU::mov}, {"MOV E,D", 5, &CPU::mov}, {"MOV E,E", 5, &CPU::mov}, {"MOV E,H", 5, &CPU::mov}, {"MOV E,L", 5, &CPU::mov}, {"MOV E,M", 7, &CPU::mov}, {"MOV E,A", 5, &CPU::mov},  // 0x50-0x5F
    {"MOV H,B", 5, &CPU::mov}, {"MOV H,C", 5, &CPU::mov}, {"MOV H,D", 5, &CPU::mov}, {"MOV H,E", 5, &CPU::mov}, {"MOV H,H", 5, &CPU::mov}, {"MOV H,L", 5, &CPU::mov}, {"MOV H,M", 7, &CPU::mov}, {"MOV H,A", 5, &CPU::mov}, {"MOV L,B", 5, &CPU::mov}, {"MOV L,C", 5, &CPU::mov}, {"MOV L,D", 5, &CPU::mov}, {"MOV L,E", 5, &CPU::mov}, {"MOV L,H", 5, &CPU::mov}, {"MOV L,L", 5, &CPU::mov}, {"MOV L,M", 7, &CPU::mov}, {"MOV L,A", 5, &CPU::mov},  // 0x60-0x6F
    {"MOV M,B", 7, &CPU::mov}, {"MOV M,C", 7, &CPU::mov}, {"MOV M,D", 7, &CPU::mov}, {"MOV M,E", 7, &CPU::mov}, {"MOV M,H", 7, &CPU::mov}, {"MOV M,L", 7, &CPU::mov}, {"HLT", 5, &CPU::hlt}, {"MOV M,A", 7, &CPU::mov}, {"MOV A,B", 5, &CPU::mov}, {"MOV A,C", 5, &CPU::mov}, {"MOV A,D", 5, &CPU::mov}, {"MOV A,E", 5, &CPU::mov}, {"MOV A,H", 5, &CPU::mov}, {"MOV A,L", 5, &CPU::mov}, {"MOV A,M", 7, &CPU::mov}, {"MOV A,A", 5, &CPU::mov},  // 0x70-0x7F
    {"ADD B", 4, &CPU::alu}, {"ADD C", 4, &CPU::alu}, {"ADD D", 4, &CPU::alu}, {"ADD E", 4, &CPU::alu}, {"ADD H", 4, &CPU::alu}, {"ADD L", 4, &CPU::alu}, {"ADD M", 7, &CPU::alu}, {"ADD A", 4, &CPU::alu}, {"ADC B", 4, &CPU::alu}, {"ADC C", 4, &CPU::alu}, {"ADC D", 4, &CPU::alu}, {"ADC E", 4, &CPU::alu}, {"ADC H", 4, &CPU::alu}, {"ADC L", 4, &CPU::alu}, {"ADC M", 7, &CPU::alu}, {"ADC A", 4, &CPU::alu},  // 0x80-0x8F
    {"SUB B", 4, &CPU::alu}, {"SUB C", 4, &CPU::alu}, {"SUB D", 4, &CPU::alu}, {"SUB E", 4, &CPU::alu}, {"SUB H", 4, &CPU::alu}, {"SUB L", 4, &CPU::alu}, {"SUB M", 7, &CPU::alu}, {"SUB A", 4, &CPU::alu}, {"SBB B", 4, &CPU::alu}, {"SBB C", 4, &CPU::alu}, {"SBB D", 4, &CPU::alu}, {"SBB E", 4, &CPU::alu}, {"SBB H", 4, &CPU::alu}, {"SBB L", 4, &CPU::alu}, {"SBB M", 7, &CPU::alu}, {"SBB A", 4, &CPU::alu},  // 0x90-0x9F
    {"ANA B", 4, &CPU::alu}, {"ANA C", 4, &CPU::alu}, {"ANA D", 4, &CPU::alu}, {"ANA E", 4, &CPU::alu}, {"ANA H", 4, &CPU::alu}, {"ANA L", 4, &CPU::alu}, {"ANA M", 7, &CPU::alu}, {"ANA A", 4, &CPU::alu}, {"XRA B", 4, &CPU::alu}, {"XRA C", 4, &CPU::alu}, {"XRA D", 4, &CPU::alu}, {"XRA E", 4, &CPU::alu}, {"XRA H", 4, &CPU::alu}, {"XRA L", 4, &CPU::alu}, {"XRA M", 7, &CPU::alu}, {"XRA A", 4, &CPU::alu},  // 0xA0-0xAF
    {"ORA B", 4, &CPU::alu}, {"ORA C", 4, &CPU::alu}, {"ORA D", 4, &CPU::alu}, {"ORA E", 4, &CPU::alu}, {"ORA H", 4, &CPU::alu}, {"ORA L", 4, &CPU::alu}, {"ORA M", 7, &CPU::alu}, {"ORA A", 4, &CPU::alu}, {"CMP B", 4, &CPU::alu}, {"CMP C", 4, &CPU::alu}, {"CMP D", 4, &CPU::alu}, {"CMP E", 4, &CPU::alu}, {"CMP H", 4, &CPU::alu}, {"CMP L", 4, &CPU::alu}, {"CMP M", 7, &CPU::alu}, {"CMP A", 4, &CPU::alu},  // 0xB0-0xBF
    {"RNZ", 5, &CPU::ret}, {"POP B", 10, &CPU::pop}, {"JNZ a16", 10, &CPU::jump}, {"JMP a16", 10, &CPU::jump}, {"CNZ a16", 11, &CPU::call}, {"PUSH B", 11, &CPU::push}, {"ADDI d8", 7, &CPU::aluImmediate}, {"RST 0", 11, &CPU::rst}, {"RZ", 5, &CPU::ret}, {"RET", 10, &CPU::ret}, {"JZ a16", 10, &CPU::jump}, {"JMP a16", 10, &CPU::jump}, {"CZ a16", 11, &CPU::call}, {"CALL a16", 17, &CPU::call}, {"ADCI d8", 7, &CPU::aluImmediate}, {"RST 1", 11, &CPU::rst},  // 0xC0-0xCF
    {"RNC", 5, &CPU::ret}, {"POP D", 10, &CPU::pop}, {"JNC a16", 10, &CPU::jump}, {"OUT d8", 10, &CPU::out}, {"CNC a16", 11, &CPU::call}, {"PUSH D", 11, &CPU::push}, {"SUBI d8", 7, &CPU::aluImmediate}, {"RST 2", 11, &CPU::rst}, {"RC", 5, &CPU::ret}, {"RET", 10, &CPU::ret}, {"JC a16", 10, &CPU::jump}, {"IN d8", 10, &CPU::in}, {"CC a16", 11, &CPU::call}, {"CALL a16", 17, &CPU::call}, {"SBBI d8", 7, &CPU::aluImmediate}, {"RST 3", 11, &CPU::rst},  // 0xD0-0xDF
    {"RPO", 5, &CPU::ret}, {"POP H", 10, &CPU::pop}, {"JPO a16", 10, &CPU::jump}, {"XTHL", 18, &CPU::xthl}, {"CPO a16", 11, &CPU::call}, {"PUSH H", 11, &CPU::push}, {"ANAI d8", 7, &CPU::aluImmediate}, {"RST 4", 11, &CPU::rst}, {"RPE", 5, &CPU::ret}, {"PCHL", 5, &CPU::pchl}, {"JPE a16", 10, &CPU::jump}, {"XCHG", 4, &CPU::xchg}, {"CPE a16", 11, &CPU::call}, {"CALL a16", 17, &CPU::call}, {"XRAI d8", 7, &CPU::aluImmediate}, {"RST 5", 11, &CPU::rst},  // 0xE0-0xEF
    {"RP", 5, &CPU::ret}, {"POP PSW", 10, &CPU::pop}, {"JP a16", 10, &CPU::jump}, {"DI", 4, &CPU::di}, {"CP a16", 11, &CPU::call}, {"PUSH PSW", 11, &CPU::push}, {"ORAI d8", 7, &CPU::aluImmediate}, {"RST 6", 11, &CPU::rst}, {"RM", 5, &CPU::ret}, {"SPHL", 5, &CPU::sphl}, {"JM a16", 10, &CPU::jump}, {"EI", 4, &CPU::ei}, {"CM a16", 11, &CPU::call}, {"CALL a16", 17, &CPU::call}, {"CMPI d8", 7, &CPU::aluImmediate}, {"RST 7", 11, &CPU::rst}  // 0xF0-0xFF
};

int CPU::executeInstruction()
{
    // reset extra cycles (only conditional CALL/RET add to this)
    m_extraCycles = 0;

    m_currentOpcode = fetchByte(); // PC is now (currentPC + 1)

    const Instruction& instr = OPCODE_TABLE[m_currentOpcode];
    (this->*instr.handler)();

    return instr.cycles + m_extraCycles;
}