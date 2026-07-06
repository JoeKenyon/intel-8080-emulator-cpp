#include <algorithm>

class Memory
{
public:
    uint8_t data[65536];


    Memory()
    {
        std::fill(std::begin(data), std::end(data), 0);
    }

    // Standard read/write
    uint8_t read(uint16_t address) const
    {
        return data[address];
    }

    void write(uint16_t address, uint8_t value)
    {
        data[address] = value;
    }

    // Specialized helpers for 16-bit word access
    uint16_t readWord(uint16_t address) const
    {
        return data[address] | (data[address + 1] << 8);
    }

    void writeWord(uint16_t address, uint16_t value) {
        data[address] = value & 0xFF;
        data[address + 1] = (value >> 8) & 0xFF;
    }
};
