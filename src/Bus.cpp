#include "Bus.h"

uint8_t Bus::readMemory(uint16_t address) const noexcept
{
    return mem.read(address);
}

void Bus::writeMemory(uint16_t address, uint8_t value) noexcept
{
    // Only block writes if they hit the actual ROM boundaries (0x0000 - 0x1FFF)
    if (address >= 0x2000)
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
    if (address >= 0x2000)
    {
        mem.write(address, value & 0xFF);
    }
    if ((address + 1) >= 0x2000)
    {
        mem.write(address + 1, (value >> 8) & 0xFF);
    }
}

void Bus::writePort(uint8_t port, uint8_t value) noexcept
{
    ports[port] = value;

    switch (port)
    {
        case 2:
            // Port 2 sets the bit shift offset (only uses the lower 3 bits)
            m_shiftOffset = value & 0b0000'0111;
            break;

        case 4:
            // Port 4 shifts the 16-bit register: moves the old high byte to the low byte,
            // and appends the new value as the new high byte.
            m_shiftRegister = (m_shiftRegister >> 8) | (static_cast<uint16_t>(value) << 8);
            break;
    }
}

uint8_t Bus::readPort(uint8_t port) noexcept
{
    switch (port)
    {
        case 3:
        {
            // Port 3 reads back the content of the shift register, shifted by the offset
            uint8_t result = static_cast<uint8_t>((m_shiftRegister >> (8 - m_shiftOffset)) & 0b1111'1111);
            return result;
        }
        default:
            // Return whatever data was sent to the port from the host system (Inputs)
            return ports[port];
    }
}
