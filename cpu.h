#include <cstdint>
#include <cstddef>

class CPU
{
public:

    CPU();
    ~CPU();

private:

    const uint8_t* rom;
    uint8_t* vram;
    uint8_t* ram;

    static const size_t MEMORY_SIZE = 65536;

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
};