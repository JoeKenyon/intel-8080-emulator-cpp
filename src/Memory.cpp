#include "Memory.h"

uint8_t Memory::read(uint16_t address) const noexcept
{
    return data[address];
}

void Memory::write(uint16_t address, uint8_t value) noexcept
{
    data[address] = value;
}

uint16_t Memory::readWord(uint16_t address) const noexcept
{
    return static_cast<uint16_t>(data[address]) | (static_cast<uint16_t>(data[address + 1]) << 8);
}

void Memory::writeWord(uint16_t address, uint16_t value) noexcept
{
    data[address] = static_cast<uint8_t>(value & 0xFF);
    data[address + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}
