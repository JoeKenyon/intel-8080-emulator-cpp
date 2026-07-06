#pragma once
#include <array>

class Memory
{
public:
    std::array<uint8_t, 0x10000> data{0};

    Memory() noexcept = default;
    ~Memory() noexcept = default;

    uint8_t read(uint16_t address) const noexcept;
    void write(uint16_t address, uint8_t value) noexcept;
    uint16_t readWord(uint16_t address) const noexcept;
    void writeWord(uint16_t address, uint16_t value) noexcept;
};
