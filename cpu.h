#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include "bus.h"
#include "opcodes.h"

// the cpu class - models the intel 8080 processor
// it reads instructions from the bus one at a time and executes them
// IO is handled via two callbacks so this file doesn't need to know
// anything about whatever machine you're plugging it into
class CPU
{
public:
    // port_in  - called when the cpu executes IN  <port>, should return the byte at that port
    // port_out - called when the cpu executes OUT <port>, should write value to that port
    using PortIn  = std::function<uint8_t(uint8_t port)>;
    using PortOut = std::function<void(uint8_t port, uint8_t value)>;

    explicit CPU(Bus& bus, PortIn port_in, PortOut port_out);

    // fetch one instruction from PC, decode it, execute it, advance PC
    void step();

    // look up the handler in the dispatch table and call it
    void execute(uint8_t opcode, uint8_t op1, uint8_t op2);

    // fire a hardware interrupt - pushes PC and jumps to n*8 (RST vector)
    // does nothing if interrupts are disabled via DI
    void generateInterrupt(int n);

    struct Registers
    {
        uint8_t A, B, C, D, E, H, L;
    };

    struct Flags
    {
        uint8_t Zero;
        uint8_t Sign;
        uint8_t Parity;
        uint8_t Carry;
        uint8_t AuxCarry;
    };

    uint16_t  PC;
    uint16_t  SP;
    Flags     flags;
    Registers regs;
    bool      interruptsEnabled = false;

private:
    Bus&    bus;
    PortIn  port_in;
    PortOut port_out;

    // all opcode handlers share this signature - opcode + up to 2 immediate bytes
    using OpHandler = void (CPU::*)(uint8_t opcode, uint8_t op1, uint8_t op2);

    // builds the 256-entry dispatch table at startup (called once, result is const)
    static std::array<OpHandler, OPCODE_NUM> buildDispatchTable();
    static const std::array<OpHandler, OPCODE_NUM> dispatch;

    // translates a 3-bit register id (from opcode bits) into a pointer to the actual register
    // ids: 0=B 1=C 2=D 3=E 4=H 5=L 6=memory[HL] 7=A  (6 is handled separately, not here)
    uint8_t* getRegisterPtr(uint8_t reg_id);

    // reads a 16-bit register pair by id: 0=BC 1=DE 2=HL 3=PSW(A+flags)
    uint16_t getRegisterPair(uint8_t pair_id);

    // writes a 16-bit value into a register pair - pair 3 unpacks into A and all flag bits
    void setRegisterPair(uint8_t pair_id, uint16_t value);

    // evaluates a condition code (bits from the opcode) against the current flags
    // used by all the conditional jumps, calls, and returns
    bool checkCondition(uint8_t condition_code);

    // the core alu logic - shared between op_alu (register source) and op_alu_immediate (byte source)
    // op_id 0=ADD 1=ADC 2=SUB 3=SBB 4=ANA 5=XRA 6=ORA 7=CMP
    void aluOp(uint8_t op_id, uint8_t src_val);

    // called for any opcode we haven't mapped yet - prints an error and carries on
    void op_unimplemented    (uint8_t opcode, uint8_t op1, uint8_t op2);

    void op_nop              (uint8_t opcode, uint8_t op1, uint8_t op2); // do nothing
    void op_hlt              (uint8_t opcode, uint8_t op1, uint8_t op2); // freeze the cpu
    void op_mov              (uint8_t opcode, uint8_t op1, uint8_t op2); // copy register to register (or memory)
    void op_mvi              (uint8_t opcode, uint8_t op1, uint8_t op2); // load an immediate byte into a register
    void op_lxi              (uint8_t opcode, uint8_t op1, uint8_t op2); // load a 16-bit immediate into a register pair
    void op_push             (uint8_t opcode, uint8_t op1, uint8_t op2); // push a register pair onto the stack
    void op_pop              (uint8_t opcode, uint8_t op1, uint8_t op2); // pop two bytes off the stack into a register pair
    void op_ldax_stax        (uint8_t opcode, uint8_t op1, uint8_t op2); // load/store A using address in BC or DE
    void op_lda_sta          (uint8_t opcode, uint8_t op1, uint8_t op2); // load/store A using a 16-bit immediate address
    void op_shld_lhld        (uint8_t opcode, uint8_t op1, uint8_t op2); // store/load HL pair directly to/from memory
    void op_jmp_conditional  (uint8_t opcode, uint8_t op1, uint8_t op2); // jump to address if condition is met (or always for JMP)
    void op_call_conditional (uint8_t opcode, uint8_t op1, uint8_t op2); // push return address then jump (conditional or CALL)
    void op_ret_conditional  (uint8_t opcode, uint8_t op1, uint8_t op2); // pop return address and jump back (conditional or RET)
    void op_rst              (uint8_t opcode, uint8_t op1, uint8_t op2); // software interrupt - push PC, jump to n*8
    void op_alu              (uint8_t opcode, uint8_t op1, uint8_t op2); // register ALU ops (ADD/ADC/SUB/SBB/ANA/XRA/ORA/CMP)
    void op_alu_immediate    (uint8_t opcode, uint8_t op1, uint8_t op2); // immediate ALU ops (ADI/ACI/SUI/SBI/ANI/XRI/ORI/CPI)
    void op_rotate           (uint8_t opcode, uint8_t op1, uint8_t op2); // bit-rotate A left or right, with or without carry
    void op_pchl             (uint8_t opcode, uint8_t op1, uint8_t op2); // jump to whatever address is in HL
    void op_inr              (uint8_t opcode, uint8_t op1, uint8_t op2); // increment a register (or memory) by 1
    void op_dcr              (uint8_t opcode, uint8_t op1, uint8_t op2); // decrement a register (or memory) by 1
    void op_inx              (uint8_t opcode, uint8_t op1, uint8_t op2); // increment a 16-bit register pair by 1
    void op_dcx              (uint8_t opcode, uint8_t op1, uint8_t op2); // decrement a 16-bit register pair by 1
    void op_dad              (uint8_t opcode, uint8_t op1, uint8_t op2); // add a register pair to HL, result goes into HL
    void op_xchg             (uint8_t opcode, uint8_t op1, uint8_t op2); // swap DE and HL
    void op_xthl             (uint8_t opcode, uint8_t op1, uint8_t op2); // swap HL with the top two bytes on the stack
    void op_sphl             (uint8_t opcode, uint8_t op1, uint8_t op2); // copy HL into SP
    void op_cma_stc_cmc      (uint8_t opcode, uint8_t op1, uint8_t op2); // CMA=flip A bits, STC=set carry, CMC=flip carry
    void op_daa              (uint8_t opcode, uint8_t op1, uint8_t op2); // decimal adjust A after BCD arithmetic
    void op_ei_di            (uint8_t opcode, uint8_t op1, uint8_t op2); // enable (EI) or disable (DI) hardware interrupts
    void op_in_out           (uint8_t opcode, uint8_t op1, uint8_t op2); // read a byte from a port (IN) or write one (OUT)
};