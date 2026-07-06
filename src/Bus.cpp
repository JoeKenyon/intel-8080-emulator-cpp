#include "Bus.h"

Bus::Bus() noexcept
{
    setupPortHandlers();
}

void Bus::setupPortHandlers() noexcept
{
    // Port 2 OUT: sets the bit shift offset (only uses the lower 3 bits)
    m_outHandlers[2] = [this](uint8_t value) noexcept
    {
        m_shiftOffset = value & 0b0000'0111;
    };

    // Port 4 OUT: shifts the 16-bit register — moves the old high byte to
    // the low byte, and appends the new value as the new high byte.
    m_outHandlers[4] = [this](uint8_t value) noexcept
    {
        m_shiftRegister = (m_shiftRegister >> 8) | (static_cast<uint16_t>(value) << 8);
    };

    // Port 3 IN: reads back the content of the shift register, shifted by the offset
    m_inHandlers[3] = [this]() noexcept -> uint8_t
    {
        return static_cast<uint8_t>((m_shiftRegister >> (8 - m_shiftOffset)) & 0b1111'1111);
    };
}

uint8_t Bus::readMemory(uint16_t address) const noexcept
{
    return mem.read(address);
}

void Bus::writeMemory(uint16_t address, uint8_t value) noexcept
{
    // Only block writes if they hit the actual ROM boundaries (0x0000 - 0x1FFF)
    if (address >= romBoundary)
    {
        mem.write(address, value);
    }
}

uint16_t Bus::readMemoryWord(uint16_t address) const noexcept
{
    return mem.readWord(address);
}

void Bus::writeMemoryWord(uint16_t address, uint16_t value) noexcept
{
    // Separately protect each byte of the word write so clipping writes are safe!
    if (address >= romBoundary)
    {
        mem.write(address, value & 0xFF);
    }
    if ((address + 1) >= romBoundary)
    {
        mem.write(address + 1, (value >> 8) & 0xFF);
    }
}

void Bus::writePort(uint8_t port, uint8_t value) noexcept
{
    ports[port] = value;

    if (m_outHandlers[port])
    {
        m_outHandlers[port](value);
    }
}

uint8_t Bus::readPort(uint8_t port) noexcept
{
    if (m_inHandlers[port])
    {
        return m_inHandlers[port]();
    }

    // No handler registered — return whatever data was sent to the port
    // from the host system (e.g. Display::handleInput writing ports 1/2).
    return ports[port];
}

void Bus::setOutHandler(uint8_t port, OutHandler handler) noexcept
{
    m_outHandlers[port] = std::move(handler);
}

void Bus::setInHandler(uint8_t port, InHandler handler) noexcept
{
    m_inHandlers[port] = std::move(handler);
}
