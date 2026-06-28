#pragma once
#include <cstdint>

// Space Invaders port map:
//
// IN ports:
//   0 - inputs (coin, P2 buttons) - mostly unused in software
//   1 - player 1 controls
//   2 - player 2 controls + DIP switches
//   3 - shift register result
//
// OUT ports:
//   2 - shift register: set shift amount (3 bits)
//   3 - sound bits (board 1)
//   4 - shift register: write new byte
//   5 - sound bits (board 2)
//   6 - watchdog (ignore)

struct InputState
{
    // Port 1 bits
    bool coin       = false;
    bool p1_start   = false;
    bool p2_start   = false;
    bool p1_fire    = false;
    bool p1_left    = false;
    bool p1_right   = false;

    // Port 2 bits
    bool p2_fire    = false;
    bool p2_left    = false;
    bool p2_right   = false;

    // DIP switches (port 2)
    uint8_t dip_lives   = 0;   // bits 0-1: extra lives setting
    bool    dip_bonus   = false;
    bool    dip_coin    = false;
};

class MachineIO
{
public:
    MachineIO();

    uint8_t readPort(uint8_t port) const;
    void    writePort(uint8_t port, uint8_t value);

    InputState input;

private:
    // Hardware shift register (16-bit internal, reads 8 bits at offset)
    uint16_t shift_register = 0;
    uint8_t  shift_amount   = 0;
};