#include "bus.h"
#include <fstream>
#include <iostream>

Bus::Bus()
{
    for (size_t i = 0; i < MEMORY_SIZE; ++i)
        memory[i] = 0;
}

bool Bus::loadRom(const std::string& filename, uint16_t startAddress)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(startAddress) + fileSize > MEMORY_SIZE)
    {
        std::cerr << "Error: ROM file is too large for the remaining memory space.\n";
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[startAddress]), fileSize))
    {
        std::cerr << "Error: Failed to read file data.\n";
        return false;
    }

    return true;
}

uint8_t Bus::read8(uint16_t addr) const
{
    return memory[addr];
}

void Bus::write8(uint16_t addr, uint8_t value)
{
    // ROM is read-only
    // if (addr <= ROM_END) return;
    memory[addr] = value;
}

uint16_t Bus::read16(uint16_t addr) const
{
    return static_cast<uint16_t>(memory[addr]) |
           (static_cast<uint16_t>(memory[(uint16_t)(addr + 1)]) << 8);
}

void Bus::write16(uint16_t addr, uint16_t value)
{
    memory[addr] = value & 0xFF;
    memory[(uint16_t)(addr + 1)] = (value >> 8) & 0xFF;
}
