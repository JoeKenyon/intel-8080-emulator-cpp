#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <iostream>
#include <vector>
#include <string>

#include "bus.h"
#include "cpu.h"
#include "machine_io.h"
#include "debug.h"

// ======================================================
// BDOS INTERCEPT (your logic)
// ======================================================
static bool interceptBDOS(CPU& cpu, Bus& bus)
{
    if (cpu.PC != 0x0005) return false;

    if (cpu.regs.C == 2)
        std::cout << (char)cpu.regs.E;

    else if (cpu.regs.C == 9)
    {
        uint16_t addr = (cpu.regs.D << 8) | cpu.regs.E;
        while (bus.read8(addr) != '$')
            std::cout << (char)bus.read8(addr++);
    }

    uint8_t lo = bus.read8(cpu.SP);
    uint8_t hi = bus.read8(cpu.SP + 1);
    cpu.PC = (hi << 8) | lo;
    cpu.SP += 2;

    return true;
}

// ======================================================
// MAIN
// ======================================================
int main(int argc, char* argv[])
{
    // ---------------- SDL ----------------
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_Window* window = SDL_CreateWindow(
        "8080 Emulator Debugger",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // ---------------- IMGUI ----------------
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF(
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        24.0f
    );

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ---------------- EMULATOR ----------------
    Bus bus;
    MachineIO io_hw;

    CPU cpu(bus,
        [&io_hw](uint8_t p){ return io_hw.readPort(p); },
        [&io_hw](uint8_t p, uint8_t v){ io_hw.writePort(p, v); });

    if (!bus.loadRom("roms/8080PRE.COM", 0x0100))
        return 1;

    cpu.PC = 0x0100;
    cpu.SP = 0xF000;

    // ---------------- DEBUG STATE ----------------
    bool running = true;
    bool paused = true;

    uint64_t instructions = 0;
    uint64_t cycles = 0;
    uint64_t frame_cycles = 0;
    int next_interrupt = 1;

    // ======================================================
    // STEP FUNCTION
    // ======================================================
    auto stepCPU = [&]()
    {
        if (cpu.PC == 0x0000)
        {
            running = false;
            return;
        }

        if (interceptBDOS(cpu, bus))
            return;

        uint8_t opcode = bus.read8(cpu.PC);
        uint8_t c = OpcodeTable[opcode].cycles;

        cpu.step();

        instructions++;
        cycles += c;
        frame_cycles += c;

        if (frame_cycles >= 16666)
        {
            cpu.generateInterrupt(next_interrupt);
            next_interrupt = (next_interrupt == 1) ? 2 : 1;
            frame_cycles -= 16666;
        }
    };

    // ======================================================
    // DISASSEMBLER
    // ======================================================
    auto disassemble = [&](uint16_t addr, int count)
    {
        struct Line { uint16_t addr; std::string text; };
        std::vector<Line> out;

        uint16_t pc = addr;

        for (int i = 0; i < count; i++)
        {
            uint8_t op = bus.read8(pc);
            auto& ins = OpcodeTable[op];

            uint8_t op1 = bus.read8(pc + 1);
            uint8_t op2 = bus.read8(pc + 2);

            char buf[64];

            if (ins.size == 1)
                snprintf(buf, sizeof(buf), "%s", ins.mnemonic);
            else if (ins.size == 2)
                snprintf(buf, sizeof(buf), "%s #%02X", ins.mnemonic, op1);
            else
                snprintf(buf, sizeof(buf), "%s #%02X%02X", ins.mnemonic, op2, op1);

            out.push_back({ pc, buf });
            pc += ins.size;
        }

        return out;
    };

    // ======================================================
    // MAIN LOOP
    // ======================================================
    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT)
                running = false;
        }

        // ---------------- KEYBOARD ----------------
        const Uint8* k = SDL_GetKeyboardState(NULL);

        static bool n_down = false;
        if (k[SDL_SCANCODE_N])
        {
            if (!n_down)
                stepCPU();
            n_down = true;
        }
        else n_down = false;

        static bool space_down = false;
        if (k[SDL_SCANCODE_SPACE])
        {
            if (!space_down)
                paused = !paused;
            space_down = true;
        }
        else space_down = false;

        // ---------------- IMGUI ----------------
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("8080 Debugger", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        float w = ImGui::GetWindowWidth();
        float leftW = w * 0.5f;

        // ======================================================
        // LEFT: DISASSEMBLY
        // ======================================================
        ImGui::BeginChild("LEFT", ImVec2(leftW, 0), true);

        ImGui::Text("DISASSEMBLY");

        auto lines = disassemble(cpu.PC - 10, 40);

        for (auto& l : lines)
        {
            if (l.addr == cpu.PC)
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1),
                    "-> %04X: %s", l.addr, l.text.c_str());
            else
                ImGui::Text("   %04X: %s", l.addr, l.text.c_str());
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // ======================================================
        // RIGHT: CPU STATE (REAL EMULATOR)
        // ======================================================
        ImGui::BeginChild("RIGHT", ImVec2(0, 0), true);

        ImGui::Text("CPU STATE");
        ImGui::Separator();

        if (ImGui::Button(paused ? "RUN (SPACE)" : "PAUSE (SPACE)"))
            paused = !paused;

        ImGui::SameLine();

        if (ImGui::Button("STEP (N)"))
            stepCPU();

        ImGui::Separator();

        // REGISTERS
        ImGui::Text("REGISTERS");
        ImGui::Text("A=%02X B=%02X C=%02X", cpu.regs.A, cpu.regs.B, cpu.regs.C);
        ImGui::Text("D=%02X E=%02X H=%02X L=%02X",
            cpu.regs.D, cpu.regs.E, cpu.regs.H, cpu.regs.L);

        ImGui::Separator();

        // FLAGS
        ImGui::Text("FLAGS");
        ImGui::Text("Z:%d S:%d P:%d C:%d AC:%d",
            cpu.flags.Zero,
            cpu.flags.Sign,
            cpu.flags.Parity,
            cpu.flags.Carry,
            cpu.flags.AuxCarry);

        ImGui::Separator();

        // EXECUTION STATE
        ImGui::Text("PC: %04X", cpu.PC);
        ImGui::Text("Instructions: %llu", instructions);
        ImGui::Text("Cycles: %llu", cycles);

        ImGui::EndChild();

        ImGui::End();

        // ---------------- EXECUTION ----------------
        if (!paused)
            stepCPU();

        // ---------------- RENDER ----------------
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
        SDL_RenderClear(renderer);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // ---------------- CLEANUP ----------------
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}