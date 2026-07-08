#include <cstddef>
#include <utility>
#include "CPU.h"

CPU::CPU(IBus& b) : bus(b)
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
    bool isConditional = (currentOpcode != 0xC3);
    uint8_t conditionCode = (currentOpcode >> 3) & 0x07;

    if (!isConditional || evaluateCondition(conditionCode))
    {
        PC = addr;
    }
}

void CPU::call()
{
    uint16_t addr = fetchWord();

    // 0xcd is the unconditional call
    bool isConditional = (currentOpcode != 0xCD);
    uint8_t conditionCode = (currentOpcode >> 3) & 0x07;

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
    bool isConditional = (currentOpcode != 0xC9);
    uint8_t conditionCode = (currentOpcode >> 3) & 0x07;

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
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;
    pushWord(getReg16(pairCode, true));
}

// handles all pop operations
void CPU::pop()
{
    // pop the word first, then route it to the right pair
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;
    uint16_t val = popWord();
    setReg16(pairCode, val, true);
}

void CPU::lxi()
{
    // grab bits 4 and 5 to figure out where this word is going
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;
    uint16_t val = fetchWord();
    setReg16(pairCode, val);
}

// handles sta, lda, shld, and lhld
void CPU::directStoreLoad()
{
    // all four instructions fetch a 16-bit memory address first
    uint16_t addr = fetchWord();

    // bit 3 tells us if we are reading (load) or writing (store)
    bool isLoad = (currentOpcode & 0x08) != 0;

    // bit 4 tells us if we are targeting the accumulator or the hl pair
    bool isAccumulator = (currentOpcode & 0x10) != 0;

    if (isAccumulator)
    {
        if (isLoad) A = bus.readByte(addr);   // lda
        else bus.writeByte(addr, A);   // sta
    }
    else
    {
        if (isLoad) HL = bus.readWord(addr); // lhld
        else bus.writeWord(addr, HL); // shld
    }
}

// handles stax b, stax d, ldax b, ldax d
void CPU::indirectStoreLoad()
{
    // grab the register pair code from bits 4 and 5
    // 0 = bc, 1 = de
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;

    // bit 3 tells us if we are reading (load) or writing (store)
    bool isLoad = (currentOpcode & 0x08) != 0;

    // fetch the memory address from that register pair
    uint16_t addr = getReg16(pairCode);

    // ldax: load memory into accumulator
    if (isLoad) A = bus.readByte(addr);
    // stax: store accumulator into memory
    else bus.writeByte(addr, A);
}

void CPU::dad()
{
    // extract pair code (0=bc, 1=de, 2=hl, 3=sp)
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;
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
    uint8_t regCode = (currentOpcode >> 3) & 0x07;
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
    uint8_t regCode = (currentOpcode >> 3) & 0x07;
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
    uint8_t destCode = (currentOpcode >> 3) & 0x07;

    // grab and write the immediate value from the instruction stream
    setReg8(destCode, fetchByte());
}

void CPU::mov()
{
    // extract destination (bits 3-5) and source (bits 0-2)
    uint8_t destCode = (currentOpcode >> 3) & 0x07;
    uint8_t srcCode  = currentOpcode & 0x07;
    setReg8(destCode, getReg8(srcCode));
}

void CPU::inx()
{
    // grab the register pair code from bits 4 and 5
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;

    // read the pair (defaults to SP for pair 3)
    // increment and write back
    setReg16(pairCode, getReg16(pairCode) + 1);
}

void CPU::dcx()
{
    // grab the register pair code from bits 4 and 5
    uint8_t pairCode = (currentOpcode >> 4) & 0x03;

    // read the pair (defaults to SP for pair 3)
    // decrement and write back
    setReg16(pairCode, getReg16(pairCode) - 1);
}

void CPU::alu()
{
    // bits 0-2 tell us where the data is coming from
    uint8_t srcCode = currentOpcode & 0x07;
    uint8_t operand = 0;

    // get the operand
    operand = getReg8(srcCode);

    // bits 3-5 tell us exactly which math operation to run
    uint8_t opType = (currentOpcode >> 3) & 0x07;

    // hand off to shared helper
    (this->*aluOps[opType])(operand);
}

void CPU::aluImmediate()
{
    // grab the byte from the program stream
    uint8_t operand = fetchByte();

    // extracts math operation from bits 3-5
    uint8_t opType = (currentOpcode >> 3) & 0x07;

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
        case 6: return bus.readByte(HL);
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
        case 6: bus.writeByte(HL, val); return;
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

void CPU::rotate()
{
    // extract rotate type from bits 3 and 4
    uint8_t rotateType = (currentOpcode >> 3) & 0x03;

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
    uint16_t rst_address = (currentOpcode & 0b00111000);
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

int CPU::executeInstruction()
{
    uint8_t opcode = fetchByte(); // PC is now (currentPC + 1)
    currentOpcode = opcode;

    // reset extra cycles
    m_extraCycles = 0;

    switch(opcode)
    {
        case 0x00:
		case 0x08:
		case 0x10:
		case 0x18:
		case 0x20:
		case 0x28:
		case 0x30:
		case 0x38:
		    nop();
			break;

		case 0x04:
		case 0x14:
		case 0x24:
		case 0x0C:
		case 0x1C:
		case 0x2C:
		case 0x3C:
		case 0x34:
		    inr();
			break;

		case 0x05:
		case 0x15:
		case 0x25:
		case 0x0D:
		case 0x1D:
		case 0x2D:
		case 0x3D:
		case 0x35:
		    dcr();
			break;

		case 0x0F:
		case 0x1F:
		case 0x07:
		case 0x17:
			rotate();
			break;

		case 0x27:
			daa();
			break;

		case 0x32:
		case 0x3A:
			directStoreLoad();
			break;

		case 0x2F:
			cma();
			break;

		case 0x37:
			stc();
			break;

		case 0x3F:
			cmc();
			break;

        case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4F:
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5F:
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x67:
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6F:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7F:
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x77:
		case 0x46:
		case 0x4E:
		case 0x56:
		case 0x5E:
		case 0x66:
		case 0x6E:
		case 0x7E:
		    mov();
			break;

		case 0x06:
		case 0x16:
		case 0x26:
		case 0x0E:
		case 0x1E:
		case 0x2E:
		case 0x3E:
		case 0x36:
		    mvi();
			break;

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8F:
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9F:
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
		case 0xA5:
		case 0xA7:
		case 0xA8:
		case 0xA9:
		case 0xAA:
		case 0xAB:
		case 0xAC:
		case 0xAD:
		case 0xAF:
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB7:
		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBF:
		case 0x86:
		case 0x8E:
		case 0x96:
		case 0x9E:
		case 0xA6:
		case 0xAE:
		case 0xB6:
		case 0xBE:
		    alu();
			break;

        case 0xC6:
        case 0xCE:
        case 0xD6:
        case 0xDE:
        case 0xE6:
        case 0xEE:
        case 0xF6:
        case 0xFE:
            aluImmediate();
            break;

        case 0xC2:
		case 0xD2:
		case 0xE2:
		case 0xF2:
		case 0xCA:
		case 0xDA:
		case 0xEA:
		case 0xFA:
		case 0xC3:
		case 0xCB:
		    jump();
			break;

		case 0xC4:
		case 0xD4:
		case 0xE4:
		case 0xF4:
		case 0xCC:
		case 0xDC:
		case 0xEC:
		case 0xFC:
		case 0xCD:
		case 0xDD:
		case 0xED:
		case 0xFD:
		    call();
			break;
		case 0xC0:
		case 0xD0:
		case 0xE0:
		case 0xF0:
		case 0xC8:
		case 0xD8:
		case 0xE8:
		case 0xF8:
		case 0xC9:
		case 0xD9:
		    ret();
			break;

		case 0x01:
		case 0x11:
		case 0x21:
		case 0x31:
			lxi();
			break;

		case 0x03:
		case 0x13:
		case 0x23:
		case 0x33:
			inx();
			break;

		case 0x0B:
		case 0x1B:
		case 0x2B:
		case 0x3B:
		    dcx();
			break;

		case 0x09:
		case 0x19:
		case 0x29:
		case 0x39:
		    dad();
			break;

		case 0x02:
		case 0x12:
		case 0x0A:
		case 0x1A:
			indirectStoreLoad();
			break;

		case 0xC1:
		case 0xD1:
		case 0xE1:
		case 0xF1:
		    pop();
			break;

		case 0xC5:
		case 0xD5:
		case 0xE5:
		case 0xF5:
		    push();
			break;

		case 0x22:
		case 0x2A:
			directStoreLoad();
			break;

		case 0xE9:
		    pchl();
			break;

		case 0xF9:
		    sphl();
			break;

		case 0xE3:
			xthl();
			break;

		case 0xEB:
			xchg();
			break;

		case 0xD3:
			out();
			break;

		case 0xDB:
			in();
			break;

		case 0xC7:
		case 0xCF:
		case 0xD7:
		case 0xDF:
		case 0xE7:
		case 0xEF:
		case 0xF7:
		case 0xFF:
			rst();
			break;

		case 0xFB:
			ei();
			break;

		case 0xF3:
			di();
			break;

		case 0x76:
			hlt();
			break;

		default:
			return 1;
    }

    return base_cycles[opcode]  + m_extraCycles;
    return 0;
}
