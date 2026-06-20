#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

class Bus
{
public:
    Bus();

    static const size_t MEMORY_SIZE = 65536;

    static const uint16_t ROM_START  = 0x0000;
    static const uint16_t ROM_END    = 0x1FFF;
    static const uint16_t RAM_START  = 0x2000;
    static const uint16_t RAM_END    = 0x23FF;
    static const uint16_t VRAM_START = 0x2400;
    static const uint16_t VRAM_END   = 0x3FFF;

    bool loadRom(const std::string& filename, uint16_t startAddress = 0x0000);

    uint8_t  read8(uint16_t addr) const;
    void     write8(uint16_t addr, uint8_t value);

    uint16_t read16(uint16_t addr) const;
    void     write16(uint16_t addr, uint16_t value);

    // direct access for debugging
    const uint8_t* raw() const { return memory; }

private:
    uint8_t memory[MEMORY_SIZE];
};
