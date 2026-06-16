#include <cstdint>
#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>

class CPU
{
public:

    CPU();
    ~CPU();

    bool loadRom(const std::string& filename, uint16_t startAddress = 0x0000);

    const uint8_t* rom;
    uint8_t* vram;
    uint8_t* ram;

    static const size_t MEMORY_SIZE = 65536;
    static const size_t ROM_SIZE = 8192;
    static const size_t RAM_SIZE = 8192;
    static const size_t VRAM_SIZE = 8192;
    static const size_t OPCODE_NUM = 256;


    uint8_t memory[MEMORY_SIZE];

    struct Registers
    {
        uint8_t A; //
        uint8_t B;
        uint8_t C;
        uint8_t D;
        uint8_t E;
        uint8_t H;
        uint8_t L;
    };

    struct Flags
    {
        uint8_t Zero     : 1;
        uint8_t Sign     : 1;
        uint8_t Parity   : 1;
        uint8_t Carry    : 1;
        uint8_t AuxCarry : 1;
    };

    uint16_t PC;
    uint16_t SP;

    Flags flags;
    Registers regs;

    struct Instruction 
    {
        const char* mnemonic;
        uint8_t size;
        uint8_t cycles;
    };

    static const Instruction OP_TABLE[OPCODE_NUM];
};