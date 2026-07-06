#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include "Memory.h"

class Bus
{
public:
    Bus() noexcept;
    ~Bus() = default;

    using OutHandler = std::function<void(uint8_t)>;
    using InHandler  = std::function<uint8_t()>;

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

    // Lets external code (e.g. a test harness) register or override a
    // port's behavior at runtime, on top of whatever setupPortHandlers()
    // wired up by default. A port with no registered handler falls back to
    // the raw ports[] array.
    void setOutHandler(uint8_t port, OutHandler handler) noexcept;
    void setInHandler(uint8_t port, InHandler handler) noexcept;

    uint16_t romBoundary = 0x2000;

private:
    // Space Invaders specific hardware bit-shift registers
    uint16_t m_shiftRegister{0};
    uint8_t m_shiftOffset{0};

    std::array<OutHandler, 256> m_outHandlers{};
    std::array<InHandler, 256>  m_inHandlers{};

    // Registers the Space Invaders shift-register handlers (ports 2/3/4).
    // Add new hardware here — one line per port, no switch statement to touch.
    void setupPortHandlers() noexcept;
};
