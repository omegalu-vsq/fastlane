#include "engine.hh"

#include <cstdint>
#include <vector>

#include <SDL2/SDL_events.h>

namespace fast {
    Engine::Engine(int width, int height) : video_(width, height) 
    {}

    static uint32_t packRGBA(const Color& c) {
    return (uint32_t(c.r) << 24) |
           (uint32_t(c.g) << 16) |
           (uint32_t(c.b) << 8)  |
           0xFFu;
    }

    void Engine::run() {
        for (int y = 0; y < video_.height_; ++y) {
            for (int x = 0; x < video_.width_; ++x) {
                if (y < video_.height_ / 2) {
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
                } else {
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE;
                }
            }
        }

        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        bool running = true;
        while (running) {
            SDL_Event event;
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            for (int i = 0; i < video_.width_ * video_.height_; ++i)
                uploadBuffer[i] = packRGBA(video_.buffer_.pixels[i]);

            SDL_UpdateTexture(
                video_.texture_,
                nullptr,
                uploadBuffer.data(),
                video_.width_ * sizeof(uint32_t)
            );

            SDL_RenderClear(video_.renderer_);
            SDL_RenderCopy(video_.renderer_, video_.texture_, nullptr, nullptr);
            SDL_RenderPresent(video_.renderer_);
        }
    }
}