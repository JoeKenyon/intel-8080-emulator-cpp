#pragma once
#include <cstdint>
#include <string_view>

class CPU
{
public:
    struct RegisterPair
    {
        uint8_t &high, &low;
        RegisterPair(uint8_t& h, uint8_t& l) : high(h), low(l) {}
        void operator=(uint16_t val) { high = (val >> 8) & 0xFF; low = val & 0xFF; }
        RegisterPair& operator++() { uint16_t val = *this; val++; *this = val; return *this; }
        RegisterPair& operator--() { uint16_t val = *this; val--; *this = val; return *this; }
        RegisterPair& operator=(const RegisterPair& other) { high = other.high; low = other.low; return *this; }
        operator uint16_t() const { return (static_cast<uint16_t>(high) << 8) | low;}
    };

    union Flags
    {
        // this struct maps directly to the byte
        struct
        {
            uint8_t Carry    : 1; // bit 0
            uint8_t _one     : 1; // bit 1 (fixed)
            uint8_t Parity   : 1; // bit 2
            uint8_t _zero2   : 1; // bit 3 (fixed)
            uint8_t AuxCarry : 1; // bit 4
            uint8_t _zero1   : 1; // bit 5 (fixed)
            uint8_t Zero     : 1; // bit 6
            uint8_t Sign     : 1; // bit 7
        };
        uint8_t packed;
    } statusFlags;

    struct
    {
        bool interruptEnabled = false;
        bool halted = false;
    } systemFlags;

    uint8_t A = 0, B = 0, C = 0, D = 0, E = 0, H = 0, L = 0;
    RegisterPair BC{B, C};
    RegisterPair DE{D, E};
    RegisterPair HL{H, L};

    uint16_t PC = 0;
    uint16_t SP = 0;

    RegisterPair PSW{A, statusFlags.packed};

    class IBus
    {
    public:
        virtual uint8_t readByte(uint16_t addr) = 0;
        virtual void writeByte(uint16_t addr, uint8_t val) = 0;
        virtual uint8_t readPort(uint8_t port) = 0;
        virtual void writePort(uint8_t port, uint8_t val) = 0;

        // 8080 standard: Little-Endian
        uint16_t readWord(uint16_t addr)
        {
            uint16_t lsb = readByte(addr);
            uint16_t msb = readByte(addr + 1);
            return (msb << 8) | lsb;
        }

        void writeWord(uint16_t addr, uint16_t val)
        {
            writeByte(addr, val & 0xFF);            // LSB to lower address
            writeByte(addr + 1, (val >> 8) & 0xFF); // MSB to higher address
        }
    };

    CPU(IBus& b);

    typedef void (CPU::*InstructionHandler)();
    InstructionHandler handlers[256];

    int executeInstruction();
    void requestInterrupt(uint8_t vector);

private:
    IBus& bus;

    struct Instruction
    {
        std::string_view mnemonic; // Lightweight, non-allocating string view
        uint8_t cycles;
        InstructionHandler handler;
    };

    static const Instruction OPCODE_TABLE[256];

    void setZeroFlag    (uint8_t value) { statusFlags.Zero = (value == 0); }
    void setSignFlag    (uint8_t value) { statusFlags.Sign = (value & 0x80) != 0; }
    void setParityFlag  (uint8_t value) { statusFlags.Parity = calculateParity(value); }
    void setCarryFlag   (uint16_t value) { statusFlags.Carry = (value > 0xFF); }
    void setHalfCarryAdd(uint8_t a, uint8_t b, uint16_t result) { statusFlags.AuxCarry = ((a ^ b ^ result) & 0x10) != 0;}
    void setHalfCarrySub(uint8_t a, uint8_t b, uint8_t result) { statusFlags.AuxCarry = ((a ^ (uint8_t)(~b) ^ result) & 0x10) != 0;}
    bool calculateParity(uint8_t value) { value ^= value >> 4; value ^= value >> 2; value ^= value >> 1; return !(value & 1);}

    uint8_t currentOpcode;

    // Extra cycles a handler adds on top of the OPCODE_TABLE base cost
    // (used for conditional CALL/RET, which take longer when the branch fires).
    int m_extraCycles{0};

    // Returns reference to 8-bit register (A, B, C, D, E, H, L)
    uint8_t getReg8(uint8_t code);
    void setReg8(uint8_t code, uint8_t val);

    // Returns value to 16bit registers/ registor pairs
    uint16_t getReg16(uint8_t code, bool stackOp = false);
    void setReg16(uint8_t code, uint16_t val, bool stackOp = false);

    uint8_t  fetchByte(){ return bus.readByte(PC++); }
    void     writeByte(uint16_t addr, uint8_t val) { bus.writeByte(addr, val); }
    uint16_t fetchWord() { uint8_t low = fetchByte(); uint8_t high = fetchByte(); return (static_cast<uint16_t>(high) << 8) | low;}
    void     writeWord(uint16_t addr, uint16_t val) { bus.writeByte(addr, val & 0xFF); bus.writeByte(addr + 1, val >> 8);}

    void alu_and(uint8_t val);
    void alu_xor(uint8_t val);
    void alu_or (uint8_t val);
    void alu_add(uint8_t val);
    void alu_adc(uint8_t val);
    void alu_sub(uint8_t val);
    void alu_sbb(uint8_t val);
    void alu_cmp(uint8_t val);

    void     pushByte(uint8_t value);
    uint8_t  popByte();
    void     pushWord(uint16_t value);
    uint16_t popWord();

    void in();
    void out();
    void jump();
    void call();
    void ret();
    bool evaluateCondition(uint8_t condCode);
    void lxi();
    void pop();
    void push();
    void directStoreLoad();
    void indirectStoreLoad();
    void xchg();
    void xthl();
    void sphl();
    void pchl();
    void dad();
    void dcr();
    void inr();
    void mov();
    void mvi();
    void dcx();
    void inx();
    void alu_(uint8_t opType, uint8_t operand);
    void alu();
    void aluImmediate();
    void rotate();
    void cma();
    void stc();
    void cmc();
    void daa();
    void rst();
    void ei();
    void di();
    void hlt();
    void nop();

    constexpr static const uint8_t  base_cycles[256] = {
    // x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE  xF
        4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4, //0x
        4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4, //1x
        4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4, //2x
        4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4, //3x
        5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, //4x
        5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, //5x
        5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5, //6x
        7,  7,  7,  7,  7,  7,  5,  7,  5,  5,  5,  5,  5,  5,  7,  5, //7x
        4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, //8x
        4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, //9x
        4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, //Ax
        4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4, //Bx
        5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11, //Cx
        5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11, //Dx
        5, 10, 10, 18, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11, //Ex
        5, 10, 10,  4, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11  //Fx
    };
};
