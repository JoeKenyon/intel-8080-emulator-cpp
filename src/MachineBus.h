#pragma once
#include "CPU.h"
#include "MachineConfig.h"
#include <array>

class MachineBus : public CPU::IBus
{
public:
    std::array<uint8_t, 0x10000> memory{};
    std::array<uint8_t, 256>     ports{};

    explicit MachineBus(const MachineConfig& config) noexcept : m_config(config) {}

    uint8_t readByte(uint16_t addr) override
    {
        return memory[addr];
    }

    void writeByte(uint16_t addr, uint8_t val) override
    {
        // ROM region is not writable on real hardware, below writableFrom is ROM.
        if (addr < m_config.writableFrom)
        {
            return;
        }
        memory[addr] = val;
    }

    uint8_t readPort(uint8_t port) override
    {
        if (m_config.customPortRead)
        {
            return m_config.customPortRead(*this, port);
        }
        return ports[port];
    }

    void writePort(uint8_t port, uint8_t val) override
    {
        if (m_config.customPortWrite)
        {
            m_config.customPortWrite(*this, port, val);
        }
    }

private:
    const MachineConfig& m_config;
};