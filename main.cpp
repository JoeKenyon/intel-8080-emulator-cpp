#include <iostream>
#include "emulator.h"

void __8080PRE()
{

}

int main()
{
    if (!loadRom("roms/cpu_tests/8080EXM.COM", 0x0100)) {
        return 1;
    }

    // 1. Setup the CP/M system call hooks right in the raw memory array
    memory[0x0000] = 0xD3; // OUT instruction
    memory[0x0001] = 0x00; // Port 0 (System Exit)

    memory[0x0005] = 0xD3; // OUT instruction
    memory[0x0006] = 0x01; // Port 1 (Console Output)
    memory[0x0007] = 0xC9; // RET instruction (Return cleanly to the test)

    // 2. Initialize your BDOS memory limit pointers for 8080EXM
    memory[0x0006] = 0x00;
    memory[0x0007] = 0xF0; // Address 0xF000

    PC = 0x0100;
    SP = 0xF000;

    while(step());

    return 0;
}
