#pragma once
#include <SDL2/SDL.h>
#include <array>
#include <cstdint>
#include <vector>
#include "MachineConfig.h"

class Display
{
public:
    ~Display();
    bool initialize(const MachineConfig& config) noexcept;
    void handleInput(std::array<uint8_t, 256>& ports, const std::vector<KeyBinding>& bindings) noexcept;
    void render(const std::array<uint8_t, 0x10000>& memory) noexcept;
    bool isOpen() const { return m_isOpen; }

private:
    int      m_width  = 224;
    int      m_height = 256;
    uint16_t m_vramStart = 0x2400;

    SDL_Window*   m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    SDL_Texture*  m_texture{nullptr};

    std::vector<uint32_t> m_screenBuffer;
    bool m_isOpen{true};
};