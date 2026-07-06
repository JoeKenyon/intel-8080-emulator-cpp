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
        std::cerr << "Failed to locate rom: " << filepath << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(&m_bus.mem.data[offset]), size))
    {
        return false;
    }
    return true;
}

bool Emulator::boot(const char* windowTitle, int windowScale) noexcept
{
    if (!m_display.initialize(windowTitle, windowScale))
    {
        return false;
    }

    // Space Invaders ROM segments mapped consecutively into system space
    if (!loadROM("./roms/invaders.h", 0x0000) ||
        !loadROM("./roms/invaders.g", 0x0800) ||
        !loadROM("./roms/invaders.f", 0x1000) ||
        !loadROM("./roms/invaders.e", 0x1800))
    {
        std::cerr << "Error: ROM load sequence interrupted.\n";
        return false;
    }

    return true;
}

void Emulator::run() noexcept
{
    auto frameStartTime = std::chrono::high_resolution_clock::now();

    while (m_display.isOpen())
    {
        m_display.handleInput(m_bus.ports);

        // HALF FRAME 1: Upper screen draw operation
        int cyclesRun = 0;
        while (cyclesRun < CYCLES_PER_HALF_FRAME)
        {
            cyclesRun += m_cpu.step();
        }

        if (m_cpu.interrupts.enabled)
        {
            m_cpu.requestInterrupt(0x08);
            m_cpu.step(); // service it immediately (matches prior timing behavior)
        }

        // HALF FRAME 2: Lower screen draw operation
        cyclesRun = 0;
        while (cyclesRun < CYCLES_PER_HALF_FRAME)
        {
            cyclesRun += m_cpu.step();
        }

        if (m_cpu.interrupts.enabled)
        {
            m_cpu.requestInterrupt(0x10);
            m_cpu.step(); // service it immediately (matches prior timing behavior)
        }

        // Update graphics buffer
        m_display.render(m_bus.mem.data);

        // Frame rate limiter (60 FPS)
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
