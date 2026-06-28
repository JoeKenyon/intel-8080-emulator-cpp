#include "cpu.h"
#include "debug.h"
#include <iostream>

// ============================================================
// setup
// ============================================================

// takes the bus for memory access and two callbacks for port IO
// that way the cpu doesn't need to know anything about the machine it's in
CPU::CPU(Bus& bus, PortIn port_in, PortOut port_out)
    : bus(bus), port_in(port_in), port_out(port_out)
{
    PC = 0x0000;
    SP = 0x0000;
    regs.A = regs.B = regs.C = regs.D = regs.E = regs.H = regs.L = 0;
    flags.Zero = flags.Sign = flags.Parity = flags.Carry = flags.AuxCarry = 0;
    interruptsEnabled = false;
}

// reads the opcode at PC, prefetches the next two bytes as potential operands,
// dumps a debug trace if enabled, advances PC by the instruction size, then executes
void CPU::step()
{
    uint8_t opcode = bus.read8(PC);
    uint8_t op1    = bus.read8((uint16_t)(PC + 1));
    uint8_t op2    = bus.read8((uint16_t)(PC + 2));

    DBG_TRACE(
        PC, opcode, OpcodeTable[opcode].mnemonic,
        regs.A, regs.B, regs.C, regs.D, regs.E, regs.H, regs.L, SP,
        flags.Zero, flags.Sign, flags.Parity, flags.Carry, flags.AuxCarry
    );

    PC += OpcodeTable[opcode].size;
    execute(opcode, op1, op2);
}

// looks up the handler for this opcode in the dispatch table and calls it
void CPU::execute(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    (this->*dispatch[opcode])(opcode, op1, op2);
}

// fires a hardware interrupt if interrupts are currently enabled
// pushes the current PC so we can RET back, then jumps to the RST vector (n * 8)
// space invaders uses RST 1 (0x0008) at mid-screen and RST 2 (0x0010) at vblank
void CPU::generateInterrupt(int n)
{
    if (!interruptsEnabled) return;
    interruptsEnabled = false;

    bus.write8(SP - 1, PC >> 8);
    bus.write8(SP - 2, PC & 0xFF);
    SP -= 2;

    PC = (uint16_t)(n * 8);
}

// ============================================================
// helpers
// ============================================================

// converts a 3-bit register id (as encoded in opcode bits) to a pointer to that register
// the 8080 encodes registers as: 0=B 1=C 2=D 3=E 4=H 5=L 7=A
// id 6 means memory[HL] which callers handle themselves before calling this
uint8_t* CPU::getRegisterPtr(uint8_t reg_id)
{
    switch (reg_id)
    {
        case 0: return &regs.B;
        case 1: return &regs.C;
        case 2: return &regs.D;
        case 3: return &regs.E;
        case 4: return &regs.H;
        case 5: return &regs.L;
        case 7: return &regs.A;
        default: return nullptr; // shouldn't happen - id 6 is memory, not a register
    }
}

// evaluates one of the 8 condition codes against the current flags
// bits from the opcode tell us which condition to check
// 0=NZ 1=Z 2=NC 3=C 4=PO 5=PE 6=P 7=M
bool CPU::checkCondition(uint8_t condition_code)
{
    switch (condition_code)
    {
        case 0: return !flags.Zero;
        case 1: return  flags.Zero;
        case 2: return !flags.Carry;
        case 3: return  flags.Carry;
        case 4: return !flags.Parity;
        case 5: return  flags.Parity;
        case 6: return !flags.Sign;
        case 7: return  flags.Sign;
        default: return false;
    }
}

// writes a 16-bit value into one of the four register pairs
// pair 3 is special - it's the PSW (processor status word), which is A + the flags byte
// the flags byte packs all five flags into specific bit positions
void CPU::setRegisterPair(uint8_t pair_id, uint16_t value)
{
    uint8_t hi = value >> 8;
    uint8_t lo = value & 0xFF;
    switch (pair_id)
    {
        case 0: regs.B = hi; regs.C = lo; break;
        case 1: regs.D = hi; regs.E = lo; break;
        case 2: regs.H = hi; regs.L = lo; break;
        case 3: // PSW - high byte is A, low byte is the flags packed into bits
            regs.A         = hi;
            flags.Sign     = (lo >> 7) & 1;
            flags.Zero     = (lo >> 6) & 1;
            flags.AuxCarry = (lo >> 4) & 1;
            flags.Parity   = (lo >> 2) & 1;
            flags.Carry    =  lo & 1;
            break;
    }
}

// reads a register pair as a 16-bit value
// pair 3 (PSW) packs A into the high byte and all flags into a specific bit layout in the low byte
// bit 1 is hardwired to 1 on the real 8080 - some software depends on this
uint16_t CPU::getRegisterPair(uint8_t pair_id)
{
    switch (pair_id)
    {
        case 0: return (regs.B << 8) | regs.C;
        case 1: return (regs.D << 8) | regs.E;
        case 2: return (regs.H << 8) | regs.L;
        case 3:
        {
            uint8_t f = (flags.Sign     << 7) |
                        (flags.Zero     << 6) |
                        (flags.AuxCarry << 4) |
                        (flags.Parity   << 2) |
                        (1              << 1) | // hardwired 1 on real hardware
                        (flags.Carry    << 0);
            return (regs.A << 8) | f;
        }
        default: return 0;
    }
}

// ============================================================
// core alu - used by both register and immediate variants
// ============================================================

// does the actual arithmetic or logic work and updates all the flags
// op_id matches the 3-bit field in opcodes: 0=ADD 1=ADC 2=SUB 3=SBB 4=ANA 5=XRA 6=ORA 7=CMP
// CMP doesn't write back to A - it just sets flags so the caller can check the result
void CPU::aluOp(uint8_t op_id, uint8_t src_val)
{
    uint8_t  a   = regs.A;
    uint16_t res = 0;

    switch (op_id)
    {
        case 0: // ADD
            res = (uint16_t)a + src_val;
            flags.Carry    = (res > 0xFF);
            flags.AuxCarry = ((a & 0x0F) + (src_val & 0x0F) > 0x0F);
            regs.A = res & 0xFF;
            break;

        case 1: // ADC
        {
            uint8_t c = flags.Carry;
            res = (uint16_t)a + src_val + c;
            flags.AuxCarry = ((a & 0x0F) + (src_val & 0x0F) + c > 0x0F);
            flags.Carry    = (res > 0xFF);
            regs.A = res & 0xFF;
            break;
        }

        case 2: // SUB
            res = (uint16_t)a - src_val;
            flags.Carry    = (a < src_val);
            flags.AuxCarry = ((a & 0x0F) < (src_val & 0x0F));
            regs.A = res & 0xFF;
            break;

        case 3: // SBB
        {
            uint8_t c = flags.Carry;
            res = (uint16_t)a - src_val - c;
            flags.AuxCarry = ((a & 0x0F) < (src_val & 0x0F) + c);
            flags.Carry    = ((int32_t)a - src_val - c < 0);
            regs.A = res & 0xFF;
            break;
        }

        case 4: // ANA
            regs.A &= src_val;
            flags.Carry    = 0;
            flags.AuxCarry = (regs.A & 0x08) ? 1 : 0;   // <--- FIXED
            res = regs.A;
            break;

        case 5: // XRA
            regs.A ^= src_val;
            flags.Carry    = 0;
            flags.AuxCarry = 0;
            res = regs.A;
            break;

        case 6: // ORA
            regs.A |= src_val;
            flags.Carry    = 0;
            flags.AuxCarry = 0;
            res = regs.A;
            break;

        case 7: // CMP
            res = (uint16_t)a - src_val;
            flags.Carry    = (a < src_val);
            flags.AuxCarry = ((a & 0x0F) < (src_val & 0x0F));
            break;
    }

    uint8_t final_val = (op_id == 7) ? (res & 0xFF) : regs.A;
    flags.Zero  = (final_val == 0);
    flags.Sign  = ((final_val & 0x80) != 0);
    uint8_t p = final_val;
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    flags.Parity = !(p & 1);
}

// ============================================================
// dispatch table
// ============================================================

// builds the 256-slot function pointer table at program startup
// every opcode maps to a handler - unimplemented ones get a fallback that prints an error
// this runs once and the result is stored as a const static
std::array<CPU::OpHandler, OPCODE_NUM> CPU::buildDispatchTable()
{
    std::array<OpHandler, OPCODE_NUM> table{};
    table.fill(&CPU::op_unimplemented);

    table[0x00] = &CPU::op_nop;

    // MOV covers the entire 0x40-0x7F block - register to register copies
    // 0x76 (HLT) sits inside that range so we override it afterward
    for (int op = 0x40; op <= 0x7F; ++op)
        table[op] = &CPU::op_mov;
    table[0x76] = &CPU::op_hlt;

    // MVI - load immediate byte into register (one per register)
    for (uint8_t op : {0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E})
        table[op] = &CPU::op_mvi;

    // LXI - load immediate 16-bit value into register pair
    for (uint8_t op : {0x01, 0x11, 0x21, 0x31})
        table[op] = &CPU::op_lxi;

    for (uint8_t op : {0xC5, 0xD5, 0xE5, 0xF5}) table[op] = &CPU::op_push;
    for (uint8_t op : {0xC1, 0xD1, 0xE1, 0xF1}) table[op] = &CPU::op_pop;

    // LDAX/STAX - load or store A via an address in BC or DE
    for (uint8_t op : {0x02, 0x0A, 0x12, 0x1A})
        table[op] = &CPU::op_ldax_stax;

    // LDA/STA - load or store A via a 16-bit immediate address
    for (uint8_t op : {0x32, 0x3A})
        table[op] = &CPU::op_lda_sta;

    // SHLD/LHLD - store or load the HL pair directly to/from memory
    for (uint8_t op : {0x22, 0x2A})
        table[op] = &CPU::op_shld_lhld;

    // the entire 0x80-0xBF block is register ALU ops
    // the top 2 bits tell us it's ALU, bits 5:3 pick the operation, bits 2:0 pick the source
    for (int op = 0x80; op <= 0xBF; ++op)
        table[op] = &CPU::op_alu;

    // immediate ALU ops - same operations but source is the next byte instead of a register
    // ADI ACI SUI SBI ANI XRI ORI CPI
    for (uint8_t op : {0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE})
        table[op] = &CPU::op_alu_immediate;

    // jumps - 0xC3 is unconditional JMP, rest are conditional
    table[0xC3] = &CPU::op_jmp_conditional;
    for (uint8_t op : {0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA})
        table[op] = &CPU::op_jmp_conditional;

    // calls - 0xCD is unconditional CALL, rest are conditional
    table[0xCD] = &CPU::op_call_conditional;
    for (uint8_t op : {0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC})
        table[op] = &CPU::op_call_conditional;

    // returns - 0xC9 is unconditional RET, rest are conditional
    table[0xC9] = &CPU::op_ret_conditional;
    for (uint8_t op : {0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8})
        table[op] = &CPU::op_ret_conditional;

    // RST 0-7 - software interrupts, jump to a fixed vector address (n * 8)
    for (uint8_t op : {0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF})
        table[op] = &CPU::op_rst;

    // RLC RRC RAL RAR - rotate A left or right with or without carry
    for (uint8_t op : {0x07, 0x0F, 0x17, 0x1F})
        table[op] = &CPU::op_rotate;

    // INR/DCR - increment or decrement a single register (or memory)
    for (uint8_t op : {0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C})
        table[op] = &CPU::op_inr;
    for (uint8_t op : {0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D})
        table[op] = &CPU::op_dcr;

    // INX/DCX - increment or decrement a full 16-bit register pair
    for (uint8_t op : {0x03, 0x13, 0x23, 0x33}) table[op] = &CPU::op_inx;
    for (uint8_t op : {0x0B, 0x1B, 0x2B, 0x3B}) table[op] = &CPU::op_dcx;

    // DAD - add a register pair to HL, only carry flag is affected
    for (uint8_t op : {0x09, 0x19, 0x29, 0x39}) table[op] = &CPU::op_dad;

    table[0xEB] = &CPU::op_xchg;        // swap DE <-> HL
    table[0xE3] = &CPU::op_xthl;        // swap HL with top of stack
    table[0xF9] = &CPU::op_sphl;        // SP = HL
    table[0xE9] = &CPU::op_pchl;        // PC = HL (indirect jump)
    table[0x2F] = &CPU::op_cma_stc_cmc; // CMA - complement accumulator
    table[0x37] = &CPU::op_cma_stc_cmc; // STC - set carry
    table[0x3F] = &CPU::op_cma_stc_cmc; // CMC - flip carry
    table[0x27] = &CPU::op_daa;         // decimal adjust after BCD add
    table[0xFB] = &CPU::op_ei_di;       // EI - enable interrupts
    table[0xF3] = &CPU::op_ei_di;       // DI - disable interrupts
    table[0xDB] = &CPU::op_in_out;      // IN  - read from port into A
    table[0xD3] = &CPU::op_in_out;      // OUT - write A to port

    return table;
}

const std::array<CPU::OpHandler, OPCODE_NUM> CPU::dispatch = CPU::buildDispatchTable();

// ============================================================
// opcode handlers
// ============================================================

// fallback for any opcode we haven't implemented - prints to stderr so it's obvious
void CPU::op_unimplemented(uint8_t opcode, uint8_t, uint8_t)
{
    std::cerr << "unimplemented opcode: 0x" << std::hex << (int)opcode
              << " (" << OpcodeTable[opcode].mnemonic << ")"
              << " at PC=0x" << (PC - OpcodeTable[opcode].size) << std::dec << "\n";
}

// NOP - literally do nothing, just burns 4 cycles
void CPU::op_nop(uint8_t, uint8_t, uint8_t) {}

// HLT - stop executing. we back up PC by 1 so we stay on the HLT instruction
// the main loop can detect this by checking if PC keeps repeating
void CPU::op_hlt(uint8_t, uint8_t, uint8_t)
{
    DBG_INFO("HLT - cpu halted");
    PC--;
}

// MOV - copy a byte between any two registers, or to/from memory[HL]
// dst and src are both encoded as 3-bit ids in the opcode byte
void CPU::op_mov(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t dst_id = (opcode >> 3) & 0x07;
    uint8_t src_id = opcode & 0x07;

    // src_id 6 means read from the memory address in HL instead of a register
    uint8_t src_val;
    if (src_id == 6)
        src_val = bus.read8((regs.H << 8) | regs.L);
    else
        src_val = *getRegisterPtr(src_id);

    // dst_id 6 means write to the memory address in HL instead of a register
    if (dst_id == 6)
        bus.write8((regs.H << 8) | regs.L, src_val);
    else
        *getRegisterPtr(dst_id) = src_val;
}

// MVI - move immediate byte into a register (or memory[HL] if dst_id is 6)
void CPU::op_mvi(uint8_t opcode, uint8_t op1, uint8_t)
{
    uint8_t dst_id = (opcode >> 3) & 0x07;
    if (dst_id == 6)
        bus.write8((regs.H << 8) | regs.L, op1);
    else
        *getRegisterPtr(dst_id) = op1;
}

// LXI - load a 16-bit immediate value into a register pair
// pair id comes from bits 5:4 of the opcode - pair 3 is SP not PSW
void CPU::op_lxi(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    uint8_t  pair_id = (opcode >> 4) & 0x03;
    uint16_t val     = (op2 << 8) | op1; // little-endian: op1 is low byte, op2 is high
    if (pair_id == 3)
        SP = val; // pair 3 means SP here, not PSW
    else
        setRegisterPair(pair_id, val);
}

// PUSH - decrement SP by 2 and write the register pair to the stack (high byte first)
void CPU::op_push(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t  pair_id = (opcode >> 4) & 0x03;
    uint16_t val     = getRegisterPair(pair_id);
    bus.write8(SP - 1, val >> 8);   // high byte at SP-1
    bus.write8(SP - 2, val & 0xFF); // low byte at SP-2
    SP -= 2;
}

// POP - read two bytes off the stack into a register pair, increment SP by 2
void CPU::op_pop(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t  pair_id = (opcode >> 4) & 0x03;
    uint8_t  lo      = bus.read8(SP);     // low byte is at the current SP
    uint8_t  hi      = bus.read8(SP + 1); // high byte is one above
    setRegisterPair(pair_id, (hi << 8) | lo);
    SP += 2;
}

// LDAX/STAX - load or store A using the address held in BC or DE
// bit 4 of the opcode picks BC (0) or DE (1), bit 3 picks load (1) or store (0)
void CPU::op_ldax_stax(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t  pair_id = (opcode >> 4) & 0x01;
    uint16_t addr    = getRegisterPair(pair_id);
    if ((opcode >> 3) & 0x01)
        regs.A = bus.read8(addr);   // LDAX
    else
        bus.write8(addr, regs.A);   // STAX
}

// LDA/STA - load or store A using a 16-bit address from the instruction bytes
// bit 3 of the opcode picks load (1) or store (0)
void CPU::op_lda_sta(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    uint16_t addr = (op2 << 8) | op1;
    if ((opcode >> 3) & 0x01)
        regs.A = bus.read8(addr);   // LDA
    else
        bus.write8(addr, regs.A);   // STA
}

// SHLD/LHLD - store HL to a memory address (SHLD) or load HL from one (LHLD)
// L goes to the lower address, H to the higher (little-endian)
void CPU::op_shld_lhld(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    uint16_t addr = (op2 << 8) | op1;
    if (opcode == 0x22) // SHLD
    {
        bus.write8(addr,     regs.L);
        bus.write8(addr + 1, regs.H);
    }
    else // LHLD (0x2A)
    {
        regs.L = bus.read8(addr);
        regs.H = bus.read8(addr + 1);
    }
}

// op_alu - register ALU operations (0x80-0xBF block)
// bits 5:3 pick the operation (ADD/ADC/SUB/etc), bits 2:0 pick the source register
void CPU::op_alu(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t op_id  = (opcode >> 3) & 0x07;
    uint8_t src_id = opcode & 0x07;
    uint8_t src_val = (src_id == 6)
        ? bus.read8((regs.H << 8) | regs.L) // src_id 6 = memory[HL]
        : *getRegisterPtr(src_id);
    aluOp(op_id, src_val);
}

// op_alu_immediate - same operations but the source is the next byte in the instruction
// the opcode bits line up perfectly: 0xC6=ADI(000) 0xCE=ACI(001) ... 0xFE=CPI(111)
void CPU::op_alu_immediate(uint8_t opcode, uint8_t op1, uint8_t)
{
    uint8_t op_id = (opcode >> 3) & 0x07;
    aluOp(op_id, op1);
}

// JMP and conditional jumps - bits 5:3 carry the condition code
// 0xC3 is unconditional JMP so we always jump for that one
void CPU::op_jmp_conditional(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    uint16_t target = (op2 << 8) | op1;
    uint8_t  cond   = (opcode >> 3) & 0x07;
    if (opcode == 0xC3 || checkCondition(cond))
        PC = target;
}

// CALL and conditional calls - push the return address then jump to the target
// 0xCD is unconditional CALL, the rest check a condition first
void CPU::op_call_conditional(uint8_t opcode, uint8_t op1, uint8_t op2)
{
    uint16_t target = (op2 << 8) | op1;
    if (opcode == 0xCD || checkCondition((opcode >> 3) & 0x07))
    {
        // PC has already advanced past this instruction so we push the correct return address
        bus.write8(SP - 1, PC >> 8);
        bus.write8(SP - 2, PC & 0xFF);
        SP -= 2;
        PC = target;
    }
}

// RET and conditional returns - pop the return address and jump back to it
// 0xC9 is unconditional RET, the rest check a condition first
void CPU::op_ret_conditional(uint8_t opcode, uint8_t, uint8_t)
{
    if (opcode == 0xC9 || checkCondition((opcode >> 3) & 0x07))
    {
        uint8_t lo = bus.read8(SP);
        uint8_t hi = bus.read8(SP + 1);
        PC = (hi << 8) | lo;
        SP += 2;
    }
}

// RST n - software interrupt, same as CALL but the target is hardcoded to n*8
// n is encoded in bits 5:3 of the opcode, giving vectors 0x00 0x08 0x10 ... 0x38
void CPU::op_rst(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t n = (opcode >> 3) & 0x07;
    bus.write8(SP - 1, PC >> 8);
    bus.write8(SP - 2, PC & 0xFF);
    SP -= 2;
    PC = n * 8;
}

// RLC/RRC/RAL/RAR - rotate A left or right by one bit
// RLC/RRC are circular (the bit that falls off comes back on the other side)
// RAL/RAR rotate through the carry flag (9-bit rotation)
void CPU::op_rotate(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t a         = regs.A;
    uint8_t old_carry = flags.Carry;

    switch (opcode)
    {
        case 0x07: // RLC - rotate left, bit 7 goes to carry and bit 0
            flags.Carry = (a >> 7) & 1;
            regs.A = (a << 1) | flags.Carry;
            break;
        case 0x0F: // RRC - rotate right, bit 0 goes to carry and bit 7
            flags.Carry = a & 1;
            regs.A = (a >> 1) | (flags.Carry << 7);
            break;
        case 0x17: // RAL - rotate left through carry (carry comes in at bit 0)
            flags.Carry = (a >> 7) & 1;
            regs.A = (a << 1) | old_carry;
            break;
        case 0x1F: // RAR - rotate right through carry (carry comes in at bit 7)
            flags.Carry = a & 1;
            regs.A = (a >> 1) | (old_carry << 7);
            break;
    }
}

// PCHL - load HL into the PC, effectively a jump to whatever address is in HL
void CPU::op_pchl(uint8_t, uint8_t, uint8_t)
{
    PC = (regs.H << 8) | regs.L;
}

// INR - increment a register (or memory[HL]) by 1
// carry flag is NOT affected, but all other flags are updated
void CPU::op_inr(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t reg_id = (opcode >> 3) & 0x07;
    uint8_t value;

    if (reg_id == 6)
    {
        uint16_t addr = (regs.H << 8) | regs.L;
        value = bus.read8(addr) + 1;
        bus.write8(addr, value);
    }
    else
    {
        uint8_t* reg = getRegisterPtr(reg_id);
        value = ++(*reg);
    }

    // auxcarry is set if there was a carry out of bit 3 (i.e. lower nibble wrapped to 0)
    flags.AuxCarry = ((value & 0x0F) == 0x00);
    flags.Zero  = (value == 0);
    flags.Sign  = ((value & 0x80) != 0);
    uint8_t p = value; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    flags.Parity = !(p & 1);
}

// DCR - decrement a register (or memory[HL]) by 1
// carry flag is NOT affected, but all other flags are updated
void CPU::op_dcr(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t reg_id = (opcode >> 3) & 0x07;
    uint8_t value;

    if (reg_id == 6) {
        uint16_t addr = (regs.H << 8) | regs.L;
        value = bus.read8(addr) - 1;
        bus.write8(addr, value);
    } else {
        uint8_t* reg = getRegisterPtr(reg_id);
        value = --(*reg);
    }

    flags.AuxCarry = ((value & 0x0F) == 0x0F);   // <-- must be == 0x0F
    flags.Zero  = (value == 0);
    flags.Sign  = ((value & 0x80) != 0);
    uint8_t p = value; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    flags.Parity = !(p & 1);
}

// INX - increment a 16-bit register pair by 1, no flags are affected at all
void CPU::op_inx(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t pair_id = (opcode >> 4) & 0x03;
    if (pair_id == 3) SP++;
    else setRegisterPair(pair_id, getRegisterPair(pair_id) + 1);
}

// DCX - decrement a 16-bit register pair by 1, no flags are affected at all
void CPU::op_dcx(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t pair_id = (opcode >> 4) & 0x03;
    if (pair_id == 3) SP--;
    else setRegisterPair(pair_id, getRegisterPair(pair_id) - 1);
}

// DAD - double add: adds a 16-bit register pair to HL, result goes back in HL
// only the carry flag is updated (set if the result overflows 16 bits)
void CPU::op_dad(uint8_t opcode, uint8_t, uint8_t)
{
    uint8_t  pair_id = (opcode >> 4) & 0x03;
    uint16_t hl      = (regs.H << 8) | regs.L;
    uint16_t rp      = (pair_id == 3) ? SP : getRegisterPair(pair_id);
    uint32_t result  = (uint32_t)hl + rp; // use 32-bit so we can detect the 16-bit overflow

    flags.Carry = (result > 0xFFFF);
    regs.H = (result >> 8) & 0xFF;
    regs.L = result & 0xFF;
}

// XCHG - swap the contents of DE and HL
void CPU::op_xchg(uint8_t, uint8_t, uint8_t)
{
    std::swap(regs.H, regs.D);
    std::swap(regs.L, regs.E);
}

// XTHL - swap HL with the two bytes at the top of the stack
// useful for temporarily stashing a value without a full push/pop
void CPU::op_xthl(uint8_t, uint8_t, uint8_t)
{
    uint8_t sp_lo = bus.read8(SP);
    uint8_t sp_hi = bus.read8(SP + 1);
    bus.write8(SP,     regs.L);
    bus.write8(SP + 1, regs.H);
    regs.L = sp_lo;
    regs.H = sp_hi;
}

// SPHL - copy HL into SP, quick way to set the stack pointer
void CPU::op_sphl(uint8_t, uint8_t, uint8_t)
{
    SP = (regs.H << 8) | regs.L;
}

// CMA/STC/CMC - three small flag/accumulator ops bundled into one handler
// CMA (0x2F) - flip every bit in A
// STC (0x37) - set the carry flag to 1
// CMC (0x3F) - toggle the carry flag
void CPU::op_cma_stc_cmc(uint8_t opcode, uint8_t, uint8_t)
{
    switch (opcode)
    {
        case 0x2F: regs.A = ~regs.A;  break;
        case 0x37: flags.Carry = 1;   break;
        case 0x3F: flags.Carry ^= 1;  break;
    }
}

// DAA - decimal adjust accumulator after a BCD addition
// adjusts A so that both nibbles are valid BCD digits (0-9)
// this is only needed for BCD arithmetic which space invaders doesn't use, but it's required for the test suite
void CPU::op_daa(uint8_t, uint8_t, uint8_t)
{
    uint8_t a = regs.A;
    uint8_t adj = 0;
    bool low_carry = false;
    bool high_carry = false;

    // Low nibble adjustment
    if ((a & 0x0F) > 9 || flags.AuxCarry) {
        adj += 0x06;
        low_carry = ((a & 0x0F) + 6) >= 0x10;
    }

    // High nibble adjustment (use updated A after low nibble add)
    uint8_t temp = a + (adj & 0x0F);
    if ((temp & 0xF0) > 0x90 || flags.Carry) {
        adj += 0x60;
        high_carry = true;
    }

    uint16_t result = a + adj;
    regs.A = result & 0xFF;
    flags.AuxCarry = low_carry ? 1 : 0;
    flags.Carry    = high_carry ? 1 : 0;

    flags.Zero  = (regs.A == 0);
    flags.Sign  = (regs.A & 0x80) != 0;
    uint8_t p = regs.A;
    p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
    flags.Parity = !(p & 1);
}

// EI/DI - enable or disable hardware interrupts
// space invaders calls EI after handling each interrupt so the next one can come in
// DI is used during critical sections where an interrupt would cause problems
void CPU::op_ei_di(uint8_t opcode, uint8_t, uint8_t)
{
    interruptsEnabled = (opcode == 0xFB); // 0xFB = EI, 0xF3 = DI
}

// IN/OUT - communicate with hardware ports via the io callbacks
// IN (0xDB) reads a byte from the port into A
// OUT (0xD3) writes A to the port
// for space invaders: ports 1/2/3 are inputs (buttons, shift register), ports 2/3/4/5 are outputs (sound, shift)
void CPU::op_in_out(uint8_t opcode, uint8_t op1, uint8_t)
{
    if (opcode == 0xDB)
        regs.A = port_in(op1);
    else
        port_out(op1, regs.A);
}