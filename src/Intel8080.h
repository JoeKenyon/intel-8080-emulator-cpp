#include <cstdint>
#include <string_view>
#include "Bus.h"

class Intel8080
{
public:

    explicit Intel8080(Bus& bus) noexcept;
    ~Intel8080() = default;

    uint16_t SP{0x0000};
    uint16_t PC{0x0000};

    struct Registers
    {
        union { uint16_t BC{0}; struct { uint8_t C; uint8_t B; }; };
        union { uint16_t DE{0}; struct { uint8_t E; uint8_t D; }; };
        union { uint16_t HL{0}; struct { uint8_t L; uint8_t H; }; };
        uint8_t A{0};
    } regs;

    struct Flags
    {
        bool Zero{false};
        bool Sign{false};
        bool Parity{false};
        bool Carry{false};
        bool AuxCarry{false};
    } flags;

    struct Instruction
    {
        std::string_view mnemonic; // Lightweight, non-allocating string view
        uint8_t cycles;
        void (Intel8080::*handler)(uint8_t);
    };

    struct InterruptState
    {
        bool enabled{false};
        bool halted{false};
        bool pending{false};
        uint8_t vector{0x00};
    } interrupts;

    static const Instruction OPCODE_TABLE[256];

    int step();

    void push16(uint16_t value) noexcept;

    // Called by the system driver (Emulator) when hardware raises an interrupt.
    // Servicing itself happens inside step(), so the CPU's own state is never
    // poked from outside.
    void requestInterrupt(uint8_t vector) noexcept;

private:

    Bus& m_bus; // Reference to the system glue

    // Extra cycles a handler adds on top of the OPCODE_TABLE base cost
    // (used for conditional CALL/RET, which take longer when the branch fires).
    int m_extraCycles{0};

    uint8_t getByte() noexcept;
    uint16_t getWord() noexcept;
    uint8_t& decodeReg(uint8_t index) noexcept;

    uint16_t pop16() noexcept;
    void doCall(uint16_t targetAddress) noexcept;
    void doReturn() noexcept;

    static constexpr bool calculateZero(uint8_t result) noexcept { return result == 0b0000'0000; }
    static constexpr bool calculateSign(uint8_t result) noexcept { return (result & 0b1000'0000) != 0b0000'0000; }
    static constexpr bool calculateParity(uint8_t result) noexcept;

    // alu helpers
    void executeAdd(uint8_t value) noexcept;
    void executeAdc(uint8_t value) noexcept;
    void executeSub(uint8_t value) noexcept;
    void executeSbb(uint8_t value) noexcept;
    void executeAna(uint8_t value) noexcept;
    void executeXra(uint8_t value) noexcept;
    void executeOra(uint8_t value) noexcept;
    void executeCmp(uint8_t value) noexcept;
    void setAluFlags(uint8_t result) noexcept;
    bool checkCondition(uint8_t cc) noexcept;

    // alu stuff
    void op_AluReg(uint8_t opcode);
    void op_AluImm(uint8_t opcode);
    void op_INR_DCR(uint8_t opcode);
    void op_INX_DCX(uint8_t opcode);
    void op_DAD(uint8_t opcode);
    void op_Rotate(uint8_t opcode);
    void op_AccControl(uint8_t opcode);

    // move stuff
    void op_MOV(uint8_t opcode);
    void op_MVI(uint8_t opcode);
    void op_LXI(uint8_t opcode);
    void op_STAX_LDAX(uint8_t opcode);
    void op_Direct16(uint8_t opcode);
    void op_PUSH(uint8_t opcode);
    void op_POP(uint8_t opcode);
    void op_XCHG_XTHL(uint8_t opcode);
    void op_PCHL_SPHL(uint8_t opcode);

    // other stuff
    void op_JmpCond(uint8_t opcode);
    void op_CallCond(uint8_t opcode);
    void op_RetCond(uint8_t opcode);
    void op_RST(uint8_t opcode);
    void op_HLT(uint8_t opcode);
    void op_Interrupt(uint8_t opcode);
    void op_NOP(uint8_t opcode);
    void op_IO(uint8_t opcode);
};
