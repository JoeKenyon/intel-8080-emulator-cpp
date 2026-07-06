#include "Emulator.h"

int main(int argc, char* argv[])
{
    Emulator emulator;

    if (emulator.boot("Intel 8080 Emulator", 3))
    {
        emulator.run();
    }

    return 0;
}
