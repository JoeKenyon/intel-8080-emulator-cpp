#pragma once
#include <cstdint>
#include <array>
#include "Memory.h"

class Bus
{
public:
    Bus() noexcept = default;
    ~Bus() = default;

    // The virtual 64KB memory space
    Memory mem;

    // Space Invaders has 8 discrete hardware I/O ports, but the 8080 can address 256
    std::array<uint8_t, 256> ports{0};

    // Unified Memory Interface
    uint8_t readMemory(uint16_t address) const noexcept;
    void writeMemory(uint16_t address, uint8_t value) noexcept;

    uint16_t readMemoryWord(uint16_t address) const noexcept;
    void writeMemoryWord(uint16_t address, uint16_t value) noexcept;

    // Unified I/O Port Interface (Space Invaders shifting hardware hides here!)
    uint8_t readPort(uint8_t port) noexcept;
    void writePort(uint8_t port, uint8_t value) noexcept;

private:
    // Space Invaders specific hardware bit-shift registers
    uint16_t m_shiftRegister{0};
    uint8_t m_shiftOffset{0};
};
