#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <array>

class Display
{
public:
    Display() noexcept = default;
    ~Display();

    // Initializes SDL2, the window, and the texture streaming buffer
    bool initialize(const char* title, int scale) noexcept;

    // Captures host keyboard events and maps them directly to Space Invaders Input Ports
    void handleInput(std::array<uint8_t, 256>& ports) noexcept;

    // Render loop: Translates the raw 8080 VRAM bitbuffer into a rotated SDL screen texture
    void render(const std::array<uint8_t, 0x10000>& memory) noexcept;

    bool isOpen() const noexcept { return m_isOpen; }

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    SDL_Texture* m_texture{nullptr};

    // Space Invaders native rotated dimensions
    static constexpr int NATIVE_WIDTH = 224;
    static constexpr int NATIVE_HEIGHT = 256;

    // Renders using a 32-bit ARGB texture format buffer
    std::array<uint32_t, NATIVE_WIDTH * NATIVE_HEIGHT> m_screenBuffer{0};
    bool m_isOpen{true};
};
