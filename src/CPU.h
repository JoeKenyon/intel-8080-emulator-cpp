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

    // Returns a reference to an 8-bit register (A, B, C, D, E, H, L)
    uint8_t& getReg8(uint8_t code);

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

    void pushByte(uint8_t value);
    uint8_t popByte();
    void pushWord(uint16_t value);
    uint16_t popWord();

    void doCall(bool condition);
    void call();
    void cc();
    void cnc();
    void cz();
    void cnz();
    void cp();
    void cm();
    void cpe();
    void cpo();

    void doReturn(bool condition);
    void ret();
    void rc();
    void rnc();
    void rz();
    void rnz();
    void rp();
    void rm();
    void rpe();
    void rpo();

    void doJump(bool condition);
    void jmp();
    void jc();
    void jnc();
    void jz();
    void jnz();
    void jp();
    void jm();
    void jpe();
    void jpo();

    void in();
    void out();

    void lxiBC();
    void lxiDE();
    void lxiHL();
    void lxiSP();

    void pushBC();
    void pushDE();
    void pushHL();
    void pushPSW();

    void popBC();
    void popDE();
    void popHL();
    void popPSW();

    void sta();
    void lda();

    void xchg();
    void xthl();
    void sphl();
    void pchl();

    void dadBC();
    void dadDE();
    void dadHL();
    void dadSP();

    void staxBC();
    void staxDE();
    void ldaxBC();
    void ldaxDE();

    void movRegToReg();
    void movRegToMem();
    void movMemToReg();
    void movImmToReg();
    void movImmToMem();

    void inrR();
    void inrM();
    void dcrR();
    void dcrM();

    void addR();
    void adcR();
    void subR();
    void sbbR();
    void anaR();
    void xraR();
    void oraR();
    void cmpR();
    void addM();
    void adcM();
    void subM();
    void sbbM();
    void anaM();
    void xraM();
    void oraM();
    void cmpM();
    void adi ();
    void aci ();
    void sui ();
    void sbi ();
    void ani ();
    void xri ();
    void ori ();
    void cpi ();
    void rlc();
    void rrc();
    void ral();
    void rar();
    void inxBC();
    void inxDE();
    void inxHL();
    void inxSP();
    void dcxBC();
    void dcxDE();
    void dcxHL();
    void dcxSP();
    void shld();
    void lhld();
    void cma();
    void stc();
    void cmc();
    void daa();
    void rst();
    void ei();
    void di();
    void hlt();
    void nop();
};
