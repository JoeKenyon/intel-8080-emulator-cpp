#include "bus.h"
#include "cpu.h"
#include <iostream>

int main()
{
    Bus bus;
    CPU cpu(bus);

    if (!bus.loadRom("roms/8080PRE.COM", 0x0100))
        return 1;

    cpu.PC = 0x0100;

    for (int i = 0; i < 10; i++)
    {
        uint8_t opcode = bus.read8(cpu.PC);
        std::cout << OpcodeTable[opcode].mnemonic << std::endl;

        cpu.step();
    }

    return 0;
}
