#pragma once
#include <SDL2/SDL.h>
#include <array>
#include <cstdint>

class Display
{
public:
    ~Display();
    bool initialize(const char* title, int scale) noexcept;
    void handleInput(std::array<uint8_t, 256>& ports) noexcept;
    void render(const std::array<uint8_t, 0x10000>& memory) noexcept;
    bool isOpen() const { return m_isOpen; }

private:
    static constexpr int NATIVE_WIDTH = 224;
    static constexpr int NATIVE_HEIGHT = 256;

    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    SDL_Texture* m_texture{nullptr};

    std::array<uint32_t, NATIVE_WIDTH * NATIVE_HEIGHT> m_screenBuffer{};
    bool m_isOpen{true};
};
