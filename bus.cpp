#include "bus.h"
#include <fstream>
#include <iostream>

// zero out all memory on startup - avoids reading garbage in uninitialized regions
Bus::Bus()
{
    for (size_t i = 0; i < MEMORY_SIZE; ++i)
        memory[i] = 0;
}

// reads a binary file off disk and copies it into memory at startAddress
// space invaders ROMs load at 0x0000, CP/M test ROMs at 0x0100
bool Bus::loadRom(const std::string& filename, uint16_t startAddress)
{
    // open at the end so we can measure the file size immediately
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "error: could not open file " << filename << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(startAddress) + fileSize > MEMORY_SIZE)
    {
        std::cerr << "error: ROM file is too large to fit in memory\n";
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[startAddress]), fileSize))
    {
        std::cerr << "error: failed to read file data\n";
        return false;
    }

    return true;
}

// reads a single byte from memory - no bounds check needed since addr is uint16_t
// and the array is exactly 65536 bytes
uint8_t Bus::read8(uint16_t addr) const
{
    return memory[addr];
}

// writes a single byte to memory
// the ROM guard is commented out for now so the test ROMs can write anywhere
// uncomment it once we're running the actual game
void Bus::write8(uint16_t addr, uint8_t value)
{
    // if (addr <= ROM_END) return; // bounce writes to ROM
    memory[addr] = value;
}

// reads two bytes and reassembles them as a little-endian 16-bit value
// if addr is 0xFFFF the high byte wraps around to 0x0000 - this is intentional
uint16_t Bus::read16(uint16_t addr) const
{
    uint16_t next = addr + 1; // intentional uint16_t wraparound at 0xFFFF
    uint8_t  lo   = memory[addr];
    uint8_t  hi   = memory[next];
    return lo | (hi << 8);
}

// writes a 16-bit value as two bytes in little-endian order (low byte first)
void Bus::write16(uint16_t addr, uint16_t value)
{
    uint16_t next  = addr + 1;
    memory[addr]   = value & 0xFF;  // low byte
    memory[next]   = value >> 8;    // high byte
}