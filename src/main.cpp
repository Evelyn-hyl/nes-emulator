#include "cartridge.hpp"
#include "frame_limiter.hpp"
#include "ppu.hpp"
#include <iostream>
#include <string>
#include <SDL.h>

int main(int argc, char* argv[]) {
    std::string rom_path = (argc > 1) ? argv[1] : "smb.nes";

    Cartridge cart(rom_path);
    PPU ppu;

    ppu.set_cartdridge(&cart);

    if (!cart.is_valid()) {
        std::cerr << "Cartridge loading failed." << "\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << "\n";
        return 1;
    }

    constexpr int DEFAULT_SCALE = 4;

    SDL_Window* window = SDL_CreateWindow(
        "Etalume NES Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        PPU::NES_WIDTH * DEFAULT_SCALE,
        PPU::NES_HEIGHT * DEFAULT_SCALE,
        0
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        PPU::NES_WIDTH,
        PPU::NES_HEIGHT
    );

    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    std::vector<uint32_t> pixel_buffer(PPU::NES_WIDTH * PPU::NES_HEIGHT, 0xFF000000);

    std::cout << "SDL canvas successfully initialized." << "\n";

    FrameLimiter frame_limiter;

    frame_limiter.start_frame();
    bool is_running = true;
    int pattern_table_index = 0;
    SDL_Event event;

    while (is_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    is_running = false;
                }
                // Hit space to toggle between character and background pattern tables
                if (event.key.keysym.sym == SDLK_SPACE) {
                    pattern_table_index = !pattern_table_index;
                }
            }
        }

        ppu.render_pattern_table(pattern_table_index, pixel_buffer.data());

        SDL_UpdateTexture(texture, nullptr, pixel_buffer.data(), PPU::NES_WIDTH * sizeof(uint32_t));

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture, nullptr, nullptr);

        SDL_RenderPresent(renderer);

        frame_limiter.end_frame();
        
        frame_limiter.start_frame();
    };

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Canvas window closed." << "\n";
    return 0;
}
