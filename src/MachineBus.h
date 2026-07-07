#pragma once
#include "CPU.h"
#include <array>

class MachineBus : public CPU::IBus
{
public:
    std::array<uint8_t, 0x10000> memory{};
    std::array<uint8_t, 256> ports{};

    // dedicated hardware for space invaders graphics math
    uint16_t shiftRegister = 0;
    uint8_t shiftOffset = 0;

    uint8_t readByte(uint16_t addr) override
    {
        return memory[addr];
    }

    void writeByte(uint16_t addr, uint8_t val) override
    {
        // rom is mapped from 0x0000 to 0x1FFF
        // writing to rom is physically impossible on the arcade board
        if (addr < 0x2000)
        {
            return;
        }

        // ram and vram mapped from 0x2000 to 0x3FFF
        memory[addr] = val;
    }

    uint8_t readPort(uint8_t port) override
    {
        switch (port)
        {
            case 1: return ports[1]; // player 1 input
            case 2: return ports[2]; // player 2 / dip switches
            case 3: return (shiftRegister >> (8 - shiftOffset)) & 0xFF; // graphics hardware
            default: return 0;
        }
    }

    void writePort(uint8_t port, uint8_t val) override
    {
        switch (port)
        {
            case 2:
                shiftOffset = val & 0x07;
                break;
            case 4:
                shiftRegister = (shiftRegister >> 8) | (static_cast<uint16_t>(val) << 8);
                break;
        }
    }
};
