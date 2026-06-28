#include "machine_io.h"

MachineIO::MachineIO()
{
    shift_register = 0;
    shift_amount   = 0;
}

uint8_t MachineIO::readPort(uint8_t port) const
{
    switch (port)
    {
        case 0:
            // Port 0: mostly not used by software, return safe value
            return 0b00001110;

        case 1:
            // Port 1:
            //   bit 0 = coin inserted
            //   bit 1 = P2 start
            //   bit 2 = P1 start
            //   bit 3 = always 1
            //   bit 4 = P1 fire
            //   bit 5 = P1 left
            //   bit 6 = P1 right
            return (uint8_t)(
                (input.coin      << 0) |
                (input.p2_start  << 1) |
                (input.p1_start  << 2) |
                (1               << 3) | // hardwired
                (input.p1_fire   << 4) |
                (input.p1_left   << 5) |
                (input.p1_right  << 6)
            );

        case 2:
            // Port 2:
            //   bits 0-1 = DIP lives
            //   bit 2    = DIP coin info
            //   bit 3    = DIP bonus
            //   bit 4    = P2 fire
            //   bit 5    = P2 left
            //   bit 6    = P2 right
            return (uint8_t)(
                (input.dip_lives & 0x03)  |
                (input.dip_coin  << 2)    |
                (input.dip_bonus << 3)    |
                (input.p2_fire   << 4)    |
                (input.p2_left   << 5)    |
                (input.p2_right  << 6)
            );

        case 3:
            // Shift register read: return top 8 bits of the 16-bit register,
            // shifted left by shift_amount
            return (uint8_t)((shift_register << shift_amount) >> 8);

        default:
            return 0xFF;
    }
}

void MachineIO::writePort(uint8_t port, uint8_t value)
{
    switch (port)
    {
        case 2:
            // Set the shift amount (only low 3 bits matter)
            shift_amount = value & 0x07;
            break;

        case 4:
            // Push new byte into shift register (shifts existing high byte down)
            shift_register = (shift_register >> 8) | ((uint16_t)value << 8);
            break;

        case 3: // Sound board 1 - stub
        case 5: // Sound board 2 - stub
        case 6: // Watchdog     - ignore
            break;
    }
}