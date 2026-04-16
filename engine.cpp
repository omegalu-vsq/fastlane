#include "engine.hh"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>

namespace fast {

    namespace {

        Color lerpColor(const Color& a, const Color& b, float t) {
            t = std::clamp(t, 0.f, 1.f);
            return Color{
                static_cast<uint8_t>(a.r + (b.r - a.r) * t),
                static_cast<uint8_t>(a.g + (b.g - a.g) * t),
                static_cast<uint8_t>(a.b + (b.b - a.b) * t),
            };
        }

        int div_tile(int v, int w) {
            return static_cast<int>(std::floor(static_cast<double>(v) / static_cast<double>(w)));
        }

        int div_tile_f(float v, float w) {
            return static_cast<int>(std::floor(v / w));
        }

        void drawSkyParallax(
            Video& video,
            int horizon_y,
            float turn
        ) {
            const Color sky_top = Colors::DARK_BLUE;
            const Color sky_mid{18, 105, 195};
            const Color cloud{120, 170, 220};
            const float denom = static_cast<float>(std::max(1, horizon_y - 1));

            for (int y = 0; y < horizon_y; ++y) {
                float depth = static_cast<float>(y) / denom;
                Color base = lerpColor(sky_top, sky_mid, depth);

                const float shift_far = turn * 88.f * depth;
                const float shift_near = turn * 28.f * depth;

                for (int x = 0; x < video.width_; ++x) {
                    float xf = static_cast<float>(x) - shift_far;
                    float xn = static_cast<float>(x) - shift_near;
                    int band_a = div_tile_f(xf, 22.f) ^ div_tile(y, 6);
                    int band_b = div_tile_f(xn, 35.f) ^ div_tile(y, 9);
                    bool cloud_pixel = ((band_a + band_b) & 1) != 0;

                    Color out = base;
                    if (cloud_pixel && depth > 0.12f) {
                        out = lerpColor(base, cloud, 0.35f + 0.25f * depth);
                    }
                    video.buffer_.pixels[x + y * video.width_] = out;
                }
            }
        }

        std::vector<float> loadTrackCenters(const std::string& path) {
            std::vector<float> centers;
            std::ifstream file(path);
            if (!file) {
                return centers;
            }

            std::string line;
            while (std::getline(file, line)) {
                // Autorise les commentaires avec '#'
                auto hash = line.find('#');
                if (hash != std::string::npos) {
                    line = line.substr(0, hash);
                }

                // Extraire tous les floats présents sur la ligne
                std::size_t i = 0;
                while (i < line.size()) {
                    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
                        ++i;
                    }
                    if (i >= line.size()) break;

                    try {
                        std::size_t consumed = 0;
                        float v = std::stof(line.substr(i), &consumed);
                        centers.push_back(v);
                        i += consumed;
                    } catch (...) {
                        // Si parsing impossible, avancer d'un caractère
                        ++i;
                    }
                }
            }

            return centers;
        }

        float sampleTrackCenter(const std::vector<float>& centers, float t) {
            if (centers.empty()) {
                return 0.5f;
            }
            if (centers.size() == 1) {
                return centers[0];
            }

            float tt = t - std::floor(t);
            float pos = tt * static_cast<float>(centers.size());
            int i0 = static_cast<int>(std::floor(pos)) % static_cast<int>(centers.size());
            int i1 = (i0 + 1) % static_cast<int>(centers.size());
            float a = pos - std::floor(pos);

            float c0 = centers[i0];
            float c1 = centers[i1];
            return c0 + (c1 - c0) * a;
        }

    }
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
        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // PARAMETERS
        float distance = 512; // between camera & screen
        float cam_height = (video_.height_ / 2); // between ground & camera
        float strip_size = 256;
        float speed = 500.;
        float dt = 0.016f;
        const float curve_tau = 0.12f;

        const int horizon_y = video_.height_ / 2 + 1 + (video_.height_ / 16);

        float roadPos = 0;
        float trackPos = 0;
        float road_curve = 0.f;

        const float track_step = 14000.f;
        const float center_to_curve = 1.f / 0.22f;
        // Lookahead en "temps de piste" (0..1) : plus petit => moins d'anticipation à l'horizon.
        const float curve_lookahead_t = 0.08f;
        const float lateral_gain = 2.2f;
        // Ralentit uniquement la progression sur le circuit (virages plus longs),
        // sans ralentir le défilement de la texture de route.
        const float track_speed_scale = 0.55f;
        const std::vector<float> track_centers = loadTrackCenters("track.txt");

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

            float track_t = trackPos / track_step;
            float center_target = sampleTrackCenter(track_centers, track_t);
            center_target = std::clamp(center_target, 0.1f, 0.9f);
            float curve_target = std::clamp((center_target - 0.5f) * center_to_curve, -1.f, 1.f);
            float blend = 1.f - std::exp(-dt / curve_tau);
            road_curve += (curve_target - road_curve) * blend;
            float current_center = std::clamp(0.5f + road_curve / center_to_curve, 0.1f, 0.9f);

            drawSkyParallax(video_, horizon_y, road_curve);

            for (int y = horizon_y; y < video_.height_; y++) {
                float z = (cam_height * distance) / (y - (video_.height_ / 2));

                float texture_position = std::fmod(z + roadPos, strip_size);
                float texture_normalized = texture_position / strip_size;

                int road_idx_vertical = static_cast<int>(std::round(static_cast<float>(sizeof(road_pattern)) * texture_normalized));

                float far = std::clamp(1.f - (zref / z), 0.f, 1.f);
                float center_far = sampleTrackCenter(track_centers, track_t + far * curve_lookahead_t);
                center_far = std::clamp(center_far, 0.1f, 0.9f);

                float center_proj = current_center + (center_far - current_center) * (far * far * far);
                // Réduit fortement le décalage près de la caméra (sinon la route sort du champ).
                float lateral = (center_proj - 0.5f) * lateral_gain * (far * far);
                float roadLeftProj = 0.5f + lateral + (((roadLeft - 0.5f) * zref) / z);
                float roadRightProj = 0.5f + lateral + (((roadRight - 0.5f) * zref) / z);

                // Sécurité: garder la route dans l'écran quand le virage est très serré.
                if (roadLeftProj < 0.f) {
                    float s = -roadLeftProj;
                    roadLeftProj += s;
                    roadRightProj += s;
                } else if (roadRightProj > 1.f) {
                    float s = 1.f - roadRightProj;
                    roadLeftProj += s;
                    roadRightProj += s;
                }

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

            float ds = speed * dt;
            roadPos += ds;
            roadPos = std::fmod(roadPos, strip_size);
            trackPos += ds * track_speed_scale;

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