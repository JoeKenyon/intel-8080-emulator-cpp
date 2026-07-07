#include "Emulator.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

Emulator::Emulator() noexcept : m_cpu(m_bus)
{
}

bool Emulator::loadROM(const char* filepath, uint16_t offset) noexcept
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        std::cerr << "failed to locate rom: " << filepath << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // read directly into the machine bus memory array
    if (!file.read(reinterpret_cast<char*>(&m_bus.memory[offset]), size))
    {
        return false;
    }
    return true;
}

bool Emulator::boot(const char* windowTitle, int windowScale) noexcept
{
    // kick off the display layer
    if (!m_display.initialize(windowTitle, windowScale))
    {
        return false;
    }

    return true;
}

void Emulator::run() noexcept
{
    auto frameStartTime = std::chrono::high_resolution_clock::now();

    while (m_display.isOpen())
    {
        // poll sdl events and map them to the hardware ports
        m_display.handleInput(m_bus.ports);

        // half frame 1: upper screen operations
        int cyclesRun = 0;
        while (cyclesRun < CYCLES_PER_HALF_FRAME)
        {
            cyclesRun += m_cpu.executeInstruction();
        }

        // fire the mid-screen hardware interrupt (rst 1)
        if (m_cpu.systemFlags.interruptEnabled)
        {
            m_cpu.requestInterrupt(0x08);
        }

        // half frame 2: lower screen operations
        cyclesRun = 0;
        while (cyclesRun < CYCLES_PER_HALF_FRAME)
        {
            cyclesRun += m_cpu.executeInstruction();
        }

        // fire the end-of-screen hardware interrupt (rst 2)
        if (m_cpu.systemFlags.interruptEnabled)
        {
            m_cpu.requestInterrupt(0x10);
        }

        // push the vram to the sdl display
        m_display.render(m_bus.memory);

        // 60hz frame rate limiter
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - frameStartTime);
        constexpr std::chrono::microseconds targetFrameTime(16666);

        if (elapsed < targetFrameTime)
        {
            std::this_thread::sleep_for(targetFrameTime - elapsed);
        }

        frameStartTime = std::chrono::high_resolution_clock::now();
    }
}
