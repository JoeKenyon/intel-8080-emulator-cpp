#include <cstdint>
#include "Memory.h"

class Intel8080
{
public:

    Intel8080();  // Constructor
    ~Intel8080(); // Destructor

    Memory mem;
    uint16_t SP;
    uint16_t PC;

    struct Registers
    {
        union { uint16_t BC; struct { uint8_t C; uint8_t B; }; };
        union { uint16_t DE; struct { uint8_t E; uint8_t D; }; };
        union { uint16_t HL; struct { uint8_t L; uint8_t H; }; };
        uint8_t A;
    } regs;

    struct Flags { uint8_t Zero, Sign, Parity, Carry, AuxCarry; } flags;

    struct Instruction
    {
        const char* mnemonic;     // For debugging/disassembly
        uint8_t cycles;           // For timing accuracy
        void (Intel8080::*handler)(uint8_t); // The instruction logic
    };

    struct InterruptState
    {
        bool enabled; // INTE flip-flop - set by EI, cleared by DI or accepted interrupt
        bool halted;  // CPU stopped by HLT, resumes on interrupt
        bool pending; // set externally when an interrupt is asserted
        uint8_t vector; // RST vector byte to execute on next step
    } interrupts;

    static const Instruction OPCODE_TABLE[256];

    uint8_t getByte();
    uint16_t getWord();
    uint8_t& decodeReg(uint8_t index);
    void push16(uint16_t value);
    uint16_t pop16();
    void doCall(uint16_t targetAddress);
    void doReturn();
    int step();

    void op_MOV(uint8_t opcode);
    void op_MVI(uint8_t opcode);
    void op_STA(uint8_t opcode);
    void op_LDA(uint8_t opcode);
    void op_STAX(uint8_t opcode);
    void op_LDAX(uint8_t opcode);
    void op_LXI_SP(uint8_t opcode);
    void op_LXI_Reg(uint8_t opcode);
    void op_SHLD(uint8_t opcode);
    void op_LHLD(uint8_t opcode);
    void op_ADD(uint8_t opcode);
    void op_ADI(uint8_t opcode);
    void op_ADC(uint8_t opcode);
    void op_ACI(uint8_t opcode);
    void op_SUB(uint8_t opcode);
    void op_SUI(uint8_t opcode);
    void op_SBB(uint8_t opcode);
    void op_SBI(uint8_t opcode);
    void op_INR(uint8_t opcode);
    void op_DCR(uint8_t opcode);
    void op_INX(uint8_t opcode);
    void op_DCX(uint8_t opcode);
    void op_DAD(uint8_t opcode);
    void op_ANA(uint8_t opcode);
    void op_ANI(uint8_t opcode);
    void op_XRA(uint8_t opcode);
    void op_XRI(uint8_t opcode);
    void op_ORA(uint8_t opcode);
    void op_ORI(uint8_t opcode);
    void op_CMP(uint8_t opcode);
    void op_CPI(uint8_t opcode);
    void op_RLC(uint8_t opcode);
    void op_RRC(uint8_t opcode);
    void op_RAL(uint8_t opcode);
    void op_RAR(uint8_t opcode);
    void op_CMA(uint8_t opcode);
    void op_STC(uint8_t opcode);
    void op_CMC(uint8_t opcode);
    void op_DAA(uint8_t opcode);
    void op_JMP(uint8_t opcode);
    void op_JC(uint8_t opcode);
    void op_JNC(uint8_t opcode);
    void op_JZ(uint8_t opcode);
    void op_JNZ(uint8_t opcode);
    void op_JM(uint8_t opcode);
    void op_JP(uint8_t opcode);
    void op_JPE(uint8_t opcode);
    void op_JPO(uint8_t opcode);
    void op_CALL(uint8_t opcode);
    void op_C(uint8_t opcode);
    void op_CC(uint8_t opcode);
    void op_CNC(uint8_t opcode);
    void op_CZ(uint8_t opcode);
    void op_CNZ(uint8_t opcode);
    void op_CM(uint8_t opcode);
    void op_CP(uint8_t opcode);
    void op_CPE(uint8_t opcode);
    void op_CPO(uint8_t opcode);
    void op_RET(uint8_t opcode);
    void op_RC(uint8_t opcode);
    void op_RNC(uint8_t opcode);
    void op_RZ(uint8_t opcode);
    void op_RNZ(uint8_t opcode);
    void op_RM(uint8_t opcode);
    void op_RP(uint8_t opcode);
    void op_RPE(uint8_t opcode);
    void op_RPO(uint8_t opcode);
    void op_RST(uint8_t opcode);
    void op_PUSH(uint8_t opcode);
    void op_PUSH_PSW(uint8_t opcode);
    void op_POP(uint8_t opcode);
    void op_POP_PSW(uint8_t opcode);
    void op_XCHG(uint8_t opcode);
    void op_XTHL(uint8_t opcode);
    void op_PCHL(uint8_t opcode);
    void op_SPHL(uint8_t opcode);
    void op_HLT(uint8_t opcode);
    void op_EI(uint8_t opcode);
    void op_DI(uint8_t opcode);
    void op_NOP(uint8_t opcode);
    void op_OUT(uint8_t opcode);
    void op_IN(uint8_t opcode);

};
