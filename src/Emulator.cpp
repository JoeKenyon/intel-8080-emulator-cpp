#include "Emulator.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

Emulator::Emulator(MachineConfig config) noexcept
    : m_config(std::move(config)), m_bus(m_config), m_cpu(m_bus)
{
}

bool Emulator::loadROMs() noexcept
{
    for (const auto& rom : m_config.roms)
    {
        std::ifstream file(rom.path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::cerr << "failed to locate rom: " << rom.path << "\n";
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (!file.read(reinterpret_cast<char*>(&m_bus.memory[rom.loadAddress]), size))
        {
            std::cerr << "failed to read rom: " << rom.path << "\n";
            return false;
        }
    }
    return true;
}

void Emulator::applyStaticPortBits() noexcept
{
    for (const auto& [port, mask] : m_config.staticPortBitsSet)
    {
        m_bus.ports[port] |= mask;
    }
    for (const auto& [port, mask] : m_config.staticPortBitsClear)
    {
        m_bus.ports[port] &= static_cast<uint8_t>(~mask);
    }
}

bool Emulator::boot() noexcept
{
    if (!loadROMs()) return false;
    return m_display.initialize(m_config);
}

void Emulator::run() noexcept
{
    const long cyclesPerFrame = m_config.cyclesPerSecond / m_config.framesPerSecond;
    const auto targetFrameTime = std::chrono::microseconds(1'000'000 / m_config.framesPerSecond);

    // interrupts must fire in the order they occur within the frame
    auto interrupts = m_config.interrupts;
    std::sort(interrupts.begin(), interrupts.end(),
              [](const FrameInterrupt& a, const FrameInterrupt& b) { return a.cyclesIntoFrame < b.cyclesIntoFrame; });

    auto frameStartTime = std::chrono::high_resolution_clock::now();

    while (m_display.isOpen())
    {
        m_display.handleInput(m_bus.ports, m_config.keyBindings);
        applyStaticPortBits();

        long cyclesRun = 0;
        size_t nextInterrupt = 0;

        while (cyclesRun < cyclesPerFrame)
        {
            cyclesRun += m_cpu.executeInstruction();

            while (nextInterrupt < interrupts.size() && cyclesRun >= interrupts[nextInterrupt].cyclesIntoFrame)
            {
                if (m_cpu.systemFlags.interruptEnabled)
                {
                    m_cpu.requestInterrupt(interrupts[nextInterrupt].vector);
                }
                nextInterrupt++;
            }
        }

        m_display.render(m_bus.memory);

        // frame rate limiter
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - frameStartTime);
        if (elapsed < targetFrameTime)
        {
            std::this_thread::sleep_for(targetFrameTime - elapsed);
        }
        frameStartTime = std::chrono::high_resolution_clock::now();
    }
}