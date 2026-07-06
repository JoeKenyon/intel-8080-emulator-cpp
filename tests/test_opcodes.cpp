#include <catch2/catch_test_macros.hpp>
#include "Bus.h"
#include "Intel8080.h"

TEST_CASE("ALU Opcodes - Arithmetic and Logic Operations", "[alu]")
{
    Bus bus;
    Intel8080 cpu(bus);

    SECTION("ADD B - basic execution and flag verification")
    {
        cpu.regs.A = 0x20;
        cpu.regs.B = 0x05;
        cpu.PC = 0x0000;
        bus.mem.write(0x0000, 0x80);

        cpu.step();

        REQUIRE(cpu.regs.A == 0x25);
        REQUIRE(cpu.flags.Zero == false);
        REQUIRE(cpu.flags.Carry == false);
    }

    SECTION("INR M - memory pointer increment validation")
    {
        cpu.PC = 0x0000;
        cpu.regs.HL = 0x2400;

        bus.mem.write(0x2400, 0x0F);
        bus.mem.write(0x0000, 0x34); // INR M

        cpu.step();

        REQUIRE(bus.mem.read(0x2400) == 0x10);
        REQUIRE(cpu.flags.AuxCarry == true);
    }

    SECTION("ADI - AuxCarry Flag Generation on Overlap")
    {
        cpu.regs.A = 0x0F;
        cpu.PC = 0x0000;

        bus.mem.write(0x0000, 0xC6); // ADI
        bus.mem.write(0x0001, 0x01);

        cpu.step();

        REQUIRE(cpu.regs.A == 0x10);
        REQUIRE(cpu.flags.AuxCarry == true);
        REQUIRE(cpu.flags.Carry == false);
    }
}

TEST_CASE("Move Opcodes - Data Transfer and Stack Frames", "[move]")
{
    Bus bus;
    Intel8080 cpu(bus);

    SECTION("PUSH and POP PSW - Flag Preservation")
    {
        cpu.regs.A = 0x8F;
        cpu.flags.Sign = true;
        cpu.flags.Zero = false;
        cpu.flags.AuxCarry = true;
        cpu.flags.Parity = false;
        cpu.flags.Carry = true;

        cpu.SP = 0x2400;
        cpu.PC = 0x0000;

        bus.mem.write(0x0000, 0xF5); // PUSH PSW
        bus.mem.write(0x0001, 0xC1); // POP B

        cpu.step(); // PUSH
        REQUIRE(cpu.SP == 0x23FE);

        cpu.regs.B = 0x00;
        cpu.regs.C = 0x00;

        cpu.step(); // POP
        REQUIRE(cpu.SP == 0x2400);
        REQUIRE(cpu.regs.B == 0x8F);
        REQUIRE(cpu.regs.C == 0x93); // Reconstructed PSW
    }
}

TEST_CASE("System Opcodes - Branching and Control Flow", "[system]")
{
    Bus bus;
    Intel8080 cpu(bus);

    SECTION("Conditional Jumps - JNZ and JZ Validation")
    {
        cpu.PC = 0x0000;

        bus.mem.write(0x0000, 0xC2); // JNZ 0xC020
        bus.mem.write(0x0001, 0x20);
        bus.mem.write(0x0002, 0xC0);

        cpu.flags.Zero = true;
        cpu.step();
        REQUIRE(cpu.PC == 0x0003); // Jump not taken (falls through)

        bus.mem.write(0x0003, 0xCA); // JZ 0xC020
        bus.mem.write(0x0004, 0x20);
        bus.mem.write(0x0005, 0xC0);

        cpu.step();
        REQUIRE(cpu.PC == 0xC020); // Jump taken
    }
}
