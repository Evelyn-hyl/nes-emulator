#include "cartridge.hpp"
#include "frame_limiter.hpp"
#include <iostream>
#include <string>
#include <SDL.h>

int main(int argc, char* argv[]) {
    std::string rom_path = (argc > 1) ? argv[1] : "smb.nes";

    Cartridge cart(rom_path);

    if (!cart.is_valid()) {
        std::cerr << "Cartridge loading failed." << "\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << "\n";
        return 1;
    }

    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;
    constexpr int DEFAULT_SCALE = 4;

    SDL_Window* window = SDL_CreateWindow(
        "Etalume NES Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        NES_WIDTH * DEFAULT_SCALE,
        NES_HEIGHT * DEFAULT_SCALE,
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
        NES_WIDTH,
        NES_HEIGHT
    );

    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
        SDL_Quit();
        return 1;
    }

    std::vector<uint32_t> pixel_buffer(NES_WIDTH * NES_HEIGHT, 0xFF000000);

    std::cout << "SDL canvas successfully initialized." << "\n";

    FrameLimiter frame_limiter;

    frame_limiter.start_frame();
    bool is_running = true;
    SDL_Event event;

    while (is_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    is_running = false;
                }
            }
        }

        // Simulated PPU output
        static uint32_t color = 0xFF0000FF;
        std::fill(pixel_buffer.begin(), pixel_buffer.end(), color++);

        SDL_UpdateTexture(texture, nullptr, pixel_buffer.data(), NES_WIDTH * sizeof(uint32_t));

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
