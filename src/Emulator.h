#pragma once
#include <cstdint>
#include <string_view>
#include "Bus.h"
#include "Intel8080.h"
#include "Display.h"

class Emulator
{
public:
    Emulator() noexcept;
    ~Emulator() = default;

    // Initializes the window and loads all consecutive ROM segments
    bool boot(const char* windowTitle, int windowScale) noexcept;

    // Runs the main 60Hz execution loop until the window is closed
    void run() noexcept;

private:
    Bus m_bus;
    Intel8080 m_cpu;
    Display m_display;

    // System constraints
    static constexpr int CYCLES_PER_FRAME = 2'000'000 / 60;
    static constexpr int CYCLES_PER_HALF_FRAME = CYCLES_PER_FRAME / 2;

    bool loadROM(const char* filepath, uint16_t offset) noexcept;
};
