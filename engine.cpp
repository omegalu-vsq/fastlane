#include "engine.hh"

#include <cmath>
#include <cstdint>
#include <vector>

#include <SDL2/SDL_timer.h>
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

    void Engine::basic() {
        for (int y = 0; y < video_.height_; ++y) {
            for (int x = 0; x < video_.width_; ++x)
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
        }

        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // PARAMETERS
        long zstart = 0;
        long ddz = 1024;
        long dzstart = 10000 * (video_.height_ / 240);
        long strip_size = 1024 * ddz * (video_.height_ / 240);

        // ENGINE LOOP
        bool running = true;
        while (running) {
            SDL_Event event;
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            long dz = 0;
            long z = zstart;
            zstart += dzstart;
            
            // NEW ROAD STATE CALCULATION
            for (int y = video_.height_ - 1; y >= video_.height_ / 2; --y) {
                int texture_position = z % strip_size;

                for (int x = 0; x < video_.width_; ++x) {
                    if (texture_position < strip_size / 2)
                        video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE;
                    else
                        video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE2;
                }

                dz += ddz;
                z += dz;
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
            //SDL_Delay(16);
        }
    }

    void Engine::projection() {
        for (int y = 0; y < video_.height_; ++y) {
            for (int x = 0; x < video_.width_; ++x)
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
        }

        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // PARAMETERS
        float distance = 512; // between camera & screen
        float cam_height = (video_.height_ / 2); // between ground & camera
        float strip_size = 256;
        float speed = 500.;
        float dt = 0.016f;

        float roadPos = 0;

        float roadWidth = 0.9f;
        // float roadLeft = (1.0f - roadWidth) / 2.0f;
        // float roadRight = roadLeft + roadWidth;
        float roadLeft = 0.5f - (roadWidth / 2.0f);
        float roadRight = 0.5f + (roadWidth / 2.0f);
        float zref = distance;

        Color road_pattern[][9] = {
            {Colors::RED, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::RED},
            {Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE},
            {Colors::RED, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::RED},
            {Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE},
            {Colors::RED, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::RED},
            {Colors::WHITE, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::GRAY, Colors::WHITE},
        };

        // ENGINE LOOP
        bool running = true;
        while (running) {
            SDL_Event event;
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            for (int y = video_.height_ / 2 + 1 + (video_.height_ / 16); y < video_.height_; y++) {
                float z = (cam_height * distance) / (y - (video_.height_ / 2));

                float texture_position = std::fmod(z + roadPos, strip_size);
                float texture_normalized = texture_position / strip_size;

                int road_idx_vertical = static_cast<int>(std::round(static_cast<float>(sizeof(road_pattern)) * texture_normalized));

                float roadLeftProj = 0.5f + (((roadLeft - 0.5f) * zref) / z);
                float roadRightProj = 0.5f + (((roadRight - 0.5f) * zref) / z);

                for (int x = 0; x < video_.width_; ++x) {
                    float x_prop = static_cast<float>(x) / static_cast<float>(video_.width_);

                    if (texture_position < strip_size / 2.)
                        video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE;
                    else
                        video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE2;

                    if (roadLeftProj <= x_prop && x_prop <= roadRightProj) {
                        float x_prop_born = (x_prop - roadLeftProj) / (roadRightProj - roadLeftProj);

                        int road_idx_horizon = static_cast<int>(std::round(static_cast<float>(sizeof(road_pattern[0])) * x_prop_born));

                        video_.buffer_.pixels[x + y * video_.width_] = road_pattern[road_idx_vertical][road_idx_horizon];
                    }

                }
            }

            roadPos += speed * dt;
            roadPos = std::fmod(roadPos, strip_size);

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
            SDL_Delay(dt * 100.0f);
        }
    }

    void Engine::power2() {
        for (int y = 0; y < video_.height_; ++y) {
            for (int x = 0; x < video_.width_; ++x)
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
        }

        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        int offset = 0;

        bool running = true;
        while (running) {
            SDL_Event event;
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }

            int y = video_.height_ / 2;
            int strip_size = 2;

            while (y < video_.height_) {
                int stripi = strip_size;

                while (y < video_.height_ && stripi > 0) {
                    for (int x = 0; x < video_.width_; ++x) {
                        if (stripi < strip_size / 2)
                            video_.buffer_.pixels[x + ((y + offset) % (video_.height_ / 2)) * video_.width_] = Colors::JADE;
                        else
                            video_.buffer_.pixels[x + ((y + offset) % (video_.height_ / 2)) * video_.width_] = Colors::JADE2;
                    }

                    y++;
                    stripi--;
                }

                strip_size *= 2;
            }

            offset++;
            offset %= video_.height_ / 2;

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
            SDL_Delay(16);
        }
    }

}