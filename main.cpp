#include <SDL2/SDL.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include "Intel8080.h" // Include your shiny new class header!

bool loadRom(Memory& mem, const std::string& filename, uint16_t startAddress)
{
    // Open the file in binary mode ("rb") and move the file pointer to the end
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open ROM file: " << filename << "\n";
        return false;
    }

    // Get the size of the ROM file based on the end position
    std::streamsize size = file.tellg();

    // Move back to the beginning of the file to read it
    file.seekg(0, std::ios::beg);

    // Read the file bytes directly into a temporary buffer
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size))
    {
        std::cerr << "Error: Failed to read ROM data from: " << filename << "\n";
        return false;
    }

    // Copy the bytes into your virtual memory array one by one
    for (std::streamsize i = 0; i < size; i++)
    {
        // Enforce 16-bit address wrapping just in case the ROM is too big
        uint16_t currentAddress = (startAddress + i) & 0xFFFF;

        mem.write(currentAddress, static_cast<uint8_t>(buffer[i]));
    }

    std::cout << "Successfully loaded " << filename << " (" << size << " bytes) at 0x"
              << std::hex << startAddress << std::dec << "\n";

    return true;
}

// ───────────────────────────────
// SCREEN
// ───────────────────────────────
static constexpr int WIDTH  = 224;
static constexpr int HEIGHT = 256;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

uint32_t framebuffer[WIDTH * HEIGHT];

// Instantiate your CPU right here globally (or locally in main)
Intel8080 cpu;

// ─── EXTENSIONS FOR YOUR PORTS ───
uint8_t port1 = 0;
uint8_t port2 = 0;

// ───────────────────────────────
// VIDEO RENDER
// ───────────────────────────────
void drawScreen()
{
    std::memset(framebuffer, 0, sizeof(framebuffer));

    // Read directly from the CPU's internalized memory layout container!
    uint8_t* video = &cpu.mem.data[0x2400]; // Or use cpu.mem.data + 0x2400 if data is public

    for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            uint8_t b = video[i * 32 + j];
            for (int bit = 0; bit < 8; bit++)
            {
                int px = i;
                int py = 255 - (j * 8 + bit);

                bool on = b & (1 << bit);
                framebuffer[py * WIDTH + px] = on ? 0xFFFFFFFF : 0xFF000000;
            }
        }
    }

    SDL_UpdateTexture(texture, nullptr, framebuffer, WIDTH * sizeof(uint32_t));
}

// ───────────────────────────────
// INPUT
// ───────────────────────────────
void handleInput()
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    port1 = 0;

    if (keys[SDL_SCANCODE_C])     port1 |= (1 << 0); // Insert Coin
    if (keys[SDL_SCANCODE_1])     port1 |= (1 << 2); // Player 1 Start
    if (keys[SDL_SCANCODE_SPACE]) port1 |= (1 << 4); // Shoot
    if (keys[SDL_SCANCODE_LEFT])  port1 |= (1 << 5); // Left
    if (keys[SDL_SCANCODE_RIGHT]) port1 |= (1 << 6); // Right
}

// ───────────────────────────────
// MAIN
// ───────────────────────────────
int main()
{
    // ─── LOAD ROMS ───
    // Note: Update your loadRom helper to write into cpu.mem instead of a raw global array!
    if (!loadRom(cpu.mem, "roms/invaders.h", 0x0000) ||
        !loadRom(cpu.mem, "roms/invaders.g", 0x0800) ||
        !loadRom(cpu.mem, "roms/invaders.f", 0x1000) ||
        !loadRom(cpu.mem, "roms/invaders.e", 0x1800))
    {
        std::cerr << "ROM load failed\n";
        return 1;
    }

    // ─── CPU INIT ───
    // The constructor already zeroes things, but you can explicitly point the Stack Pointer
    cpu.PC = 0x0000;
    cpu.SP = 0xF000;

    // ─── SDL INIT ───
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        "8080 Space Invaders",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH * 3,
        HEIGHT * 3,
        SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT);

    bool running = true;
    const double FPS = 60.0;

    // Switch from counting arbitrary instructions to tracking micro-cycles accurately!
    const int CYCLES_PER_FRAME = 2000000 / FPS;
    const int CYCLES_PER_HALF_FRAME = CYCLES_PER_FRAME / 2;

    // ─── MAIN LOOP ───
    while (running)
    {
        uint64_t frameStart = SDL_GetTicks64();

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) running = false;
        }

        handleInput();

        // ─── FIRST HALF OF SCREEN EXECUTION ───
        int cycles = 0;
        while (cycles < CYCLES_PER_HALF_FRAME)
        {
            // step() now accurately returns 4, 7, 10 etc. based on the real opcode execution timing
            cycles += cpu.step();
        }

        // Trigger Mid-Screen Interrupt (RST 1) via your internal class state container
        if (cpu.interrupts.enabled) {
            cpu.interrupts.pending = true;
            cpu.interrupts.vector = 0xCF; // Opcode for RST 1 is 0xCF (Vector address 0x0008)
        }

        // ─── SECOND HALF OF SCREEN EXECUTION ───
        while (cycles < CYCLES_PER_FRAME)
        {
            cycles += cpu.step();
        }

        // Trigger VBLANK End-Of-Screen Interrupt (RST 2)
        if (cpu.interrupts.enabled) {
            cpu.interrupts.pending = true;
            cpu.interrupts.vector = 0xD7; // Opcode for RST 2 is 0xD7 (Vector address 0x0010)
        }

        // ─── RENDER ───
        drawScreen();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // ─── FRAMERATE LOCK (60 FPS) ───
        uint64_t frameTime = SDL_GetTicks64() - frameStart;
        if (frameTime < 16)
        {
            SDL_Delay(16 - frameTime);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
