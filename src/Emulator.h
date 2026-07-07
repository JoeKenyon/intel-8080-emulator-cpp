#pragma once
#include "CPU.h"
#include "MachineBus.h"
#include "Display.h"

class Emulator
{
public:
    Emulator() noexcept;
    bool loadROM(const char* filepath, uint16_t offset) noexcept;
    bool boot(const char* windowTitle, int windowScale) noexcept;
    void run() noexcept;

private:
    // 2 MHz cpu / 60 frames per second / 2 half-frames
    static constexpr int CYCLES_PER_HALF_FRAME = 16666;

    MachineBus m_bus;
    CPU m_cpu;
    Display m_display;
};
