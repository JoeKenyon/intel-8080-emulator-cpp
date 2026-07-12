#pragma once
#include "CPU.h"
#include "../ui/Display.h"
#include "MachineBus.h"
#include "MachineConfig.h"

class Emulator
{
public:
    explicit Emulator(MachineConfig config) noexcept;

    bool boot() noexcept;
    void run() noexcept;

private:
    bool loadROMs() noexcept;
    void applyStaticPortBits() noexcept;

    MachineConfig m_config;
    MachineBus    m_bus;
    CPU           m_cpu;
    Display       m_display;
};
