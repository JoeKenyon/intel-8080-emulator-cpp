#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

// the bus owns all 64KB of memory and is the only thing that touches it
// the cpu talks to everything (rom, ram, vram) by going through here
// for now it's one flat array - later you could add separate read/write
// behaviour per region (e.g. writes to ROM bounce off)
class Bus
{
public:
    Bus();

    static const size_t MEMORY_SIZE = 65536; // full 16-bit address space

    // space invaders memory map:
    //   0x0000 - 0x1FFF  ROM (the game code, read-only)
    //   0x2000 - 0x23FF  RAM (general purpose working memory)
    //   0x2400 - 0x3FFF  VRAM (what gets drawn to the screen)
    static const uint16_t ROM_START  = 0x0000;
    static const uint16_t ROM_END    = 0x1FFF;
    static const uint16_t RAM_START  = 0x2000;
    static const uint16_t RAM_END    = 0x23FF;
    static const uint16_t VRAM_START = 0x2400;
    static const uint16_t VRAM_END   = 0x3FFF;

    // loads a binary file into memory at the given start address
    // returns false if the file can't be opened or doesn't fit
    bool loadRom(const std::string& filename, uint16_t startAddress = 0x0000);

    // read/write a single byte
    uint8_t read8(uint16_t addr) const;
    void    write8(uint16_t addr, uint8_t value);

    // read/write a 16-bit little-endian value (low byte first)
    uint16_t read16(uint16_t addr) const;
    void     write16(uint16_t addr, uint16_t value);

    // direct pointer to the raw memory - used by the renderer to read VRAM
    const uint8_t* raw() const { return memory; }

private:
    uint8_t memory[MEMORY_SIZE];
};