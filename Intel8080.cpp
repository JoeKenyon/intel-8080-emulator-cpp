#include "Intel8080.h"
#include <iostream>

Intel8080::Intel8080(Bus& bus) noexcept : m_bus(bus)
{
    PC = 0;
    SP = 0;
    regs.A = 0;
    regs.BC = 0;
    regs.DE = 0;
    regs.HL = 0;
}

int Intel8080::step()
{
    // Service any pending interrupt before fetching the next instruction.
    // This keeps all CPU state changes (PC, SP, enabled/halted flags) inside
    // the CPU itself rather than being poked from Emulator::run().
    if (interrupts.pending && interrupts.enabled)
    {
        interrupts.pending = false;
        interrupts.enabled = false;
        interrupts.halted = false;
        push16(PC);
        PC = interrupts.vector;
        return 11; // Equivalent to an RST instruction's cycle cost
    }

    if (interrupts.halted)
    {
        return 1; // Processor burns 1 cycle per step while halted
    }

    // Fetch the next instruction byte boundary
    uint16_t currentPC = PC; // Save the PC before we increment it
    uint8_t opcode = getByte();

    if (currentPC >= 0x0000 && currentPC <= 0x0100)
        {
            std::cout << "BOOT TRACE -> PC: 0x" << std::hex << currentPC
                        << " Opcode: 0x" << (int)opcode
                        << " SP: 0x" << SP
                        << " A: 0x" << (int)regs.A
                        << " HL: 0x" << regs.HL << std::dec << "\n";
    }

    // Retrieve instruction telemetry records from your multiplexed table
    const Instruction& inst = OPCODE_TABLE[opcode];

    // Fire off the target multiplex member function pointer handler!
    m_extraCycles = 0;
    if (inst.handler != nullptr)
    {
        (this->*inst.handler)(opcode);
    }

    return inst.cycles + m_extraCycles;
}

void Intel8080::requestInterrupt(uint8_t vector) noexcept
{
    interrupts.pending = true;
    interrupts.vector = vector;
}

uint8_t Intel8080::getByte() noexcept
{
    uint8_t byte = m_bus.readMemory(PC);
    PC++;
    return byte;
}

uint16_t Intel8080::getWord() noexcept
{
    // Little-endian layout: Low byte fetched first, then high byte
    uint8_t low = getByte();
    uint8_t high = getByte();
    return (static_cast<uint16_t>(high) << 8) | low;
}

uint8_t& Intel8080::decodeReg(uint8_t index) noexcept
{
    // Maps 3-bit architecture instruction indexes back to specific structural variables
    switch (index)
    {
        case 0b000: return regs.B;
        case 0b001: return regs.C;
        case 0b010: return regs.D;
        case 0b011: return regs.E;
        case 0b100: return regs.H;
        case 0b101: return regs.L;
        case 0b111: default: return regs.A;
    }
}
