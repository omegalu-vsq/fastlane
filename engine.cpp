#include "engine.hh"
#include "texture.hh"

#include <cmath>
#include <cstdint>
#include <iostream>
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

    void Engine::scanline() {
        for (int y = 0; y < video_.height_; ++y) {
            for (int x = 0; x < video_.width_; ++x)
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
        }

        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // PARAMETERS
        float distance = 512; // between camera & screen
        float cam_height = (video_.height_ / 2); // between ground & camera
        float strip_size = 512;
        float speed = 500.0f;
        float dt = 0.016f;

        // ACCUMULATOR
        float roadPos = 0;

        // ROAD SIZE CALCULATION
        float roadWidth = 1.5f;
        // float roadLeft = (1.0f - roadWidth) / 2.0f;
        // float roadRight = roadLeft + roadWidth;
        float lateral_pos = 0.5f;
        float roadLeft = lateral_pos - (roadWidth / 2.0f);
        float roadRight = lateral_pos + (roadWidth / 2.0f);
        float zref = distance;
        float pivot = 0.5f;

        // ROAD TEXTURE
        Texture road_pattern = Texture::load("textures/basicroad.txt");
        road_pattern.flip_vertical();
        int pattern_height = road_pattern.height_;
        int pattern_width = road_pattern.width_;

        // DECORATION SPRITES
        SpriteZone spriteZone = SpriteZone::load("textures/romzone.txt");
        spriteZone.setRoadOffset(0.15f);
        spriteZone.setWidthScale(2.0f);

        std::vector<std::vector<bool>> spritedL(spriteZone.height_, std::vector<bool>(spriteZone.width_, false));
        std::vector<std::vector<bool>> spritedR(spriteZone.height_, std::vector<bool>(spriteZone.width_, false));

        float spriteZoneLR = roadLeft - spriteZone.roadOffset_; // right border of the left sprite zone
        float spriteZoneLL = spriteZoneLR - spriteZone.widthScale_; // left border of the left sprite zone
        float spriteZoneRL = roadRight + spriteZone.roadOffset_; // left border of the right sprite zone
        float spriteZoneRR = spriteZoneRL + spriteZone.widthScale_; // right border of the right sprite zone

        // ENGINE LOOP
        bool running = true;
        while (running) {
            SDL_Event event;
            
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT)
                    running = false;
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS)
                        speed += 100.0f;
                    else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)
                        speed -= 100.0f;
                    else if (event.key.keysym.sym == SDLK_RIGHT)
                        pivot -= 0.05f;
                    else if (event.key.keysym.sym == SDLK_LEFT)
                        pivot += 0.05f;
                    else if (event.key.keysym.sym == SDLK_z)
                        cam_height += 10.0f;
                    else if (event.key.keysym.sym == SDLK_s)
                        cam_height -= 10.0f;
                    else if (event.key.keysym.sym == SDLK_q)
                        lateral_pos += 0.1f;
                    else if (event.key.keysym.sym == SDLK_d)
                        lateral_pos -= 0.1f;
                    else if (event.key.keysym.sym == SDLK_c) {
                        lateral_pos = 0.5f;
                        pivot = 0.5f;
                        cam_height = video_.height_ / 2;
                        speed = 500.0f;
                    }
                }
            }

            roadLeft = lateral_pos - (roadWidth / 2.0f);
            roadRight = lateral_pos + (roadWidth / 2.0f);

            spriteZoneLR = roadLeft - spriteZone.roadOffset_;
            spriteZoneLL = spriteZoneLR - spriteZone.widthScale_;
            spriteZoneRL = roadRight + spriteZone.roadOffset_;
            spriteZoneRR = spriteZoneRL + spriteZone.widthScale_;

            // sky clear
            for (int y = 0; y < video_.height_; ++y) {
                for (int x = 0; x < video_.width_; ++x)
                        video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
            }

            int lastStripIdx = -1;

            for (int y = video_.height_ / 2 + 1/* + (video_.height_ / 16)*/; y < video_.height_ + (video_.height_ / 4) ; y++) {
                float z = (cam_height * distance) / (y - (video_.height_ / 2));

                int stripIdx = static_cast<int>((z + roadPos) / strip_size);

                if (stripIdx != lastStripIdx) {
                    lastStripIdx = stripIdx;
                    for (auto& row : spritedL) std::fill(row.begin(), row.end(), false);
                    for (auto& row : spritedR) std::fill(row.begin(), row.end(), false);
                }

                float texture_position = std::fmod(z + roadPos, strip_size);
                float texture_normalized = texture_position / strip_size;

                //std::cout << texture_normalized << std::endl;

                int road_idx_vertical = static_cast<int>(static_cast<float>(pattern_height) * texture_normalized);
                if (road_idx_vertical >= pattern_height)
                    road_idx_vertical--;

                int spritezone_idx_vertical = static_cast<int>((static_cast<float>(spriteZone.height_) * texture_normalized));
                if (spritezone_idx_vertical >= spriteZone.height_)
                    spritezone_idx_vertical--;
                if (spritezone_idx_vertical <= 0)
                    spritezone_idx_vertical = 0;

                //std::cout << road_idx_vertical << std::endl;

                float roadLeftProj = pivot + (((roadLeft - pivot) * zref) / z);
                float roadRightProj = pivot + (((roadRight - pivot) * zref) / z);

                float spriteZoneLRProj = pivot + (((spriteZoneLR - pivot) * zref) / z);
                float spriteZoneLLProj = pivot + (((spriteZoneLL - pivot) * zref) / z);
                float spriteZoneRLProj = pivot + (((spriteZoneRL - pivot) * zref) / z);
                float spriteZoneRRProj = pivot + (((spriteZoneRR - pivot) * zref) / z);

                for (int x = 0 - (video_.width_ / 4); x < video_.width_ + (video_.width_ / 4); x++) {
                    // std::cout << '(' << x << ", " << y << ")\n";
                    // std::cout << spriteZone.width_ / 8<< "\n";
                    float x_prop = static_cast<float>(x) / static_cast<float>(video_.width_ - 1);

                    // STRIP DRAWING
                    if (0 <= y && y < video_.height_ && 0 <= x && x < video_.width_) {
                        if (texture_position < strip_size / 2.)
                            video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE;
                        else
                            video_.buffer_.pixels[x + y * video_.width_] = Colors::JADE2;
                    }

                    // ROAD DRAWING
                    if (roadLeftProj <= x_prop && x_prop <= roadRightProj) {
                        float x_prop_born = (x_prop - roadLeftProj) / (roadRightProj - roadLeftProj);

                        int road_idx_horizon = static_cast<int>(static_cast<float>(pattern_width) * x_prop_born);
                        if (road_idx_horizon >= pattern_width)
                            road_idx_horizon--;

                        //std::cout << '[' << road_idx_vertical << "][" << road_idx_horizon << ']' << std::endl;

                        if (0 <= y && y < video_.height_ && 0 <= x && x < video_.width_)
                            video_.buffer_.pixels[x + y * video_.width_] = road_pattern.get(road_idx_horizon, road_idx_vertical);
                    }

                    // SIDE SPRITE DRAWING
                    if (spriteZoneLLProj <= x_prop && x_prop <= spriteZoneLRProj) {
                        float x_prop_born = (x_prop - spriteZoneLLProj) / (spriteZoneLRProj - spriteZoneLLProj);

                        float cellwidth = (spriteZoneLRProj - spriteZoneLLProj) / static_cast<float>(spriteZone.width_);

                        int spritezone_idx_horizon = static_cast<int>(static_cast<float>(spriteZone.width_) * x_prop_born);
                        if (spritezone_idx_horizon >= spriteZone.width_)
                            spritezone_idx_horizon--;
                        if (spritezone_idx_horizon <= 0)
                            spritezone_idx_horizon = 0;

                        int xcell = static_cast<int>(std::round((spriteZoneLLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                        if (!spritedL[spritezone_idx_vertical][spritezone_idx_horizon] && spriteZone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                            Sprite sprite = spriteZone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                            int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * zref * sprite.scale_ / z);
                            int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                            int top = y - spriteHeight;
                            int left = xcell - (spriteWidth / 2);

                            for (int y_spr = top; y_spr < y; y_spr++) {
                                float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);

                                int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);

                                if (0 <= y_spr && y_spr < video_.height_) {

                                    for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                        if (0 <= x_spr && x_spr < video_.width_) {
                                            float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);

                                            int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);

                                            Color painter = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                            if (painter != Colors::TRANSPARENT)
                                                video_.buffer_.pixels[x_spr + y_spr * video_.width_] = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                        }

                                    }
                                }
                            }

                            spritedL[spritezone_idx_vertical][spritezone_idx_horizon] = true;
                        }

                    }
                    else if (spriteZoneRLProj <= x_prop && x_prop <= spriteZoneRRProj) {
                        float x_prop_born = (x_prop - spriteZoneRLProj) / (spriteZoneRRProj - spriteZoneRLProj);

                        float cellwidth = (spriteZoneRRProj - spriteZoneRLProj) / static_cast<float>(spriteZone.width_);

                        int spritezone_idx_horizon = static_cast<int>(static_cast<float>(spriteZone.width_) * x_prop_born);
                        if (spritezone_idx_horizon >= spriteZone.width_)
                            spritezone_idx_horizon--;
                        if (spritezone_idx_horizon <= 0)
                            spritezone_idx_horizon = 0;

                        int xcell = static_cast<int>(std::round((spriteZoneRLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                        if (!spritedR[spritezone_idx_vertical][spritezone_idx_horizon] && spriteZone.get(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                            Sprite sprite = spriteZone.get(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                            int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * zref * sprite.scale_ / z);
                            int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                            int top = y - spriteHeight;
                            int left = xcell - (spriteWidth / 2);

                            for (int y_spr = top; y_spr < y; y_spr++) {
                                float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);

                                int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);

                                if (0 <= y_spr && y_spr < video_.height_) {

                                    for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                        if (0 <= x_spr && x_spr < video_.width_) {

                                            float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);

                                            int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);

                                            Color painter = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                            if (painter != Colors::TRANSPARENT)
                                                video_.buffer_.pixels[x_spr + y_spr * video_.width_] = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                        }

                                    }
                                }
                            }

                            spritedR[spritezone_idx_vertical][spritezone_idx_horizon] = true;
                        }

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

    void Engine::projection2() {
        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // PARAMETERS
        float distance = 512.0f; // between camera & screen
        float cam_height = (video_.height_ / 2); // between ground & camera
        float strip_size = 512.0f;
        float speed = 500.0f;
        float dt = 0.016f;

        // ACCUMULATOR
        float roadPos = 0.0f;

        // ROAD PARAMETERS
        float roadWidth = 1.5f;
        float lateral_pos = 0.5f;
        float pivot = 0.5f;
        float zref = distance;

        // ROAD TEXTURE
        Texture road_pattern = Texture::load("textures/basicroad.txt");
        road_pattern.flip_vertical();
        int pattern_height = road_pattern.height_;
        int pattern_width = road_pattern.width_;

       // DECORATION SPRITES
        SpriteZone spriteZone = SpriteZone::load("textures/sunfield.txt");
        spriteZone.setRoadOffset(0.15f);
        spriteZone.setWidthScale(2.0f);

        std::vector<std::vector<bool>> spritedL(spriteZone.height_, std::vector<bool>(spriteZone.width_, false));
        std::vector<std::vector<bool>> spritedR(spriteZone.height_, std::vector<bool>(spriteZone.width_, false));

        //float segmentCount = spriteZone.height_;
        
        // SEGMENT PIPELINE PARAMETERS
        float segmentLength = strip_size / spriteZone.height_; // distance between segment
        int drawDistance = 500; // number of segments to draw

        auto clampInt = [](int v, int lo, int hi) {
            return (v < lo) ? lo : (v > hi ? hi : v);
        };

        // SDL LOOP
        bool running = true;
        while (running) {
            SDL_Event event;

            // KEYBOARD MAPPING
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS)
                        speed += 100.0f;
                    else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)
                        speed -= 100.0f;
                    else if (event.key.keysym.sym == SDLK_RIGHT)
                        pivot -= 0.05f;
                    else if (event.key.keysym.sym == SDLK_LEFT)
                        pivot += 0.05f;
                    else if (event.key.keysym.sym == SDLK_z)
                        cam_height += 10.0f;
                    else if (event.key.keysym.sym == SDLK_s)
                        cam_height -= 10.0f;
                    else if (event.key.keysym.sym == SDLK_q)
                        lateral_pos += 0.1f;
                    else if (event.key.keysym.sym == SDLK_d)
                        lateral_pos -= 0.1f;
                    else if (event.key.keysym.sym == SDLK_c) {
                        lateral_pos = 0.5f;
                        pivot = 0.5f;
                        cam_height = video_.height_ / 2.0f;
                        speed = 500.0f;
                    }
                }
            }

            // NORMALIZED ROAD & SPRITEZONE BORDERS
            float roadLeft = lateral_pos - (roadWidth / 2.0f);
            float roadRight = lateral_pos + (roadWidth / 2.0f);

            float spriteZoneLR = roadLeft - spriteZone.roadOffset_; // right border of the left sprite zone
            float spriteZoneLL = spriteZoneLR - spriteZone.widthScale_; // left border of the left sprite zone
            float spriteZoneRL = roadRight + spriteZone.roadOffset_; // left border of the right sprite zone
            float spriteZoneRR = spriteZoneRL + spriteZone.widthScale_; // right border of the right sprite zone


            // SKY CLEAR
            for (int y = 0; y < video_.height_; ++y) {
                for (int x = 0; x < video_.width_; ++x) {
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
                }
            }

            int laststripIdx = -1;

            float horizon = video_.height_ / 2.0f;

            float cameraOffset = std::fmod(roadPos, segmentLength);
            if (cameraOffset < 0.0f)
                cameraOffset += segmentLength;

            // ENGINE LOOP
            for (int i = drawDistance; i >= 1; --i) {
                float zFar = (i * segmentLength) - cameraOffset;
                float zNear = ((i - 1) * segmentLength) - cameraOffset;

                if (zNear < 2.0f)
                    zNear = 2.0f;
                if (zFar <= zNear)
                    continue;

                // y_screen bounds computation
                float screenYFar = horizon + (distance * cam_height) / zFar;
                float screenYNear = horizon + (distance * cam_height) / zNear;

                int yTop = static_cast<int>(std::round(screenYFar));
                int yBottom = static_cast<int>(std::round(screenYNear));

                if (yTop > yBottom)
                    continue;

                if (yBottom < 0 || yTop >= video_.height_)
                    continue;

                int yStart = clampInt(yTop, 0, video_.height_ - 1);
                int yEnd = clampInt(yBottom, 0, video_.height_ - 1);

                if (yEnd < yStart)
                     continue;

                // Road borders
                float roadLeftFarProj  = pivot + (((roadLeft  - pivot) * zref) / zFar);
                float roadRightFarProj = pivot + (((roadRight - pivot) * zref) / zFar);

                float roadLeftNearProj  = pivot + (((roadLeft  - pivot) * zref) / zNear);
                float roadRightNearProj = pivot + (((roadRight - pivot) * zref) / zNear);

                // SpriteZone flags reset
                int stripIdx = static_cast<int>((zFar + roadPos) / strip_size);
                if (stripIdx != laststripIdx) {
                    laststripIdx = stripIdx;
                    for (auto& row : spritedL) std::fill(row.begin(), row.end(), false);
                    for (auto& row : spritedR) std::fill(row.begin(), row.end(), false);

                }

                Color grassColor = stripIdx % 2 == 0 ? Colors::JADE : Colors::JADE2;
                // Color roadColor  = stripIdx % 2 == 0 ? Colors::DARK_GRAY : Colors::GRAY;
                
                // INTERPOLATION
                for (int y = yStart; y <= yEnd; y++) {
                    float y_prop = yEnd == yStart ? 0.5f : (static_cast<float>(y) - screenYFar) / (screenYNear - screenYFar);

                    if (y_prop < 0.0f) y_prop = 0.0f;
                    if (y_prop > 1.0f) y_prop = 1.0f;

                    float leftProj  = roadLeftFarProj + (roadLeftNearProj - roadLeftFarProj) * y_prop;
                    float rightProj = roadRightFarProj + (roadRightNearProj - roadRightFarProj) * y_prop;

                    float projWidth = rightProj - leftProj;
                    if (projWidth <= 1e-6f) {
                        for (int x = 0; x < video_.width_; ++x)
                            video_.buffer_.pixels[x + y * video_.width_] = grassColor;
                        continue;
                    }

                    if (rightProj <= 0.0f || leftProj >= 1.0f) {
                        for (int x = 0; x < video_.width_; ++x)
                            video_.buffer_.pixels[x + y * video_.width_] = grassColor;
                        continue;
                    }

                    int leftPix = static_cast<int>(std::round(leftProj * static_cast<float>(video_.width_ - 1)));
                    int rightPix = static_cast<int>(std::round(rightProj * static_cast<float>(video_.width_ - 1)));

                    if (leftPix > rightPix)
                        std::swap(leftPix, rightPix);

                    // road
                    int xStart = clampInt(leftPix, 0, video_.width_ - 1);
                    int xEnd = clampInt(rightPix, 0, video_.width_ - 1);

                    if (xEnd < xStart) {
                        for (int x = 0; x < video_.width_; ++x)
                            video_.buffer_.pixels[x + y * video_.width_] = grassColor;
                        continue;
                    }

                    for (int x = 0; x < xStart; ++x)
                        video_.buffer_.pixels[x + y * video_.width_] = grassColor;
                    for (int x = xEnd + 1; x < video_.width_; ++x)
                        video_.buffer_.pixels[x + y * video_.width_] = grassColor;

                    float invZFar = 1.0f / zFar;
                    float invZNear = 1.0f / zNear;
                    float invZ = invZFar + (invZNear - invZFar) * y_prop;
                    if (invZ <= 1e-6f)
                        continue;

                    float zSample = 1.0f / invZ;
                    float vPos = std::fmod(zSample + roadPos, strip_size);
                    if (vPos < 0.0f)
                        vPos += strip_size;

                    float vNorm = vPos / strip_size;
                    int road_idx_vertical = static_cast<int>(vNorm * static_cast<float>(pattern_height));
                    road_idx_vertical = clampInt(road_idx_vertical, 0, pattern_height - 1);

                    // int road_idx_vertical = static_cast<int>(((static_cast<float>(segmentIdx) + y_prop) / segmentCount) * static_cast<float>(pattern_height));
                    // if (road_idx_vertical >= pattern_height) road_idx_vertical--;
                    // if (road_idx_vertical < 0) road_idx_vertical = 0;

                    //std::cout << static_cast<float>(segmentIdx) << std::endl;
                    //std::cout << y_prop << std::endl;

                    for (int x = xStart; x <= xEnd; x++) {
                        float x_prop = ((static_cast<float>(x) / (video_.width_ - 1)) - leftProj) / projWidth;

                        if (x_prop < 0.0f) x_prop = 0.0f;
                        if (x_prop > 1.0f) x_prop = 1.0f;

                        //std::cout << x_prop << std::endl;

                        int road_idx_horizon = static_cast<int>(x_prop * static_cast<float>(pattern_width));
                        road_idx_horizon = clampInt(road_idx_horizon, 0, pattern_width - 1);
                        //std::cout << road_idx_horizon << std::endl;

                        if (0 <= x && x < video_.width_ && 0 <= y && y < video_.height_)
                            video_.buffer_.pixels[x + y * video_.width_] = road_pattern.get(road_idx_horizon, road_idx_vertical);

                    }
                }

                // SPRITE ZONE
                if (i < drawDistance / 8) {
                    float zMid = 2 / ((1.0f / zFar) + (1.0f / zNear));
                    float screenYMid = horizon + (distance * cam_height) / zMid;

                    int segmentIdx = static_cast<int>(std::floor(std::fmod(zMid + roadPos, strip_size) / segmentLength));

                    int yMid = static_cast<int>(std::round(screenYMid));
                    yMid = clampInt(yMid, 0, video_.height_ - 1);
                    float spriteZoneLRProj = pivot + (((spriteZoneLR - pivot) * zref) / zMid);
                    float spriteZoneLLProj = pivot + (((spriteZoneLL - pivot) * zref) / zMid);
                    float spriteZoneRLProj = pivot + (((spriteZoneRL - pivot) * zref) / zMid);
                    float spriteZoneRRProj = pivot + (((spriteZoneRR - pivot) * zref) / zMid);

                    int spritezone_idx_vertical = segmentIdx;

                    // LEFT
                    if (spriteZoneLLProj < spriteZoneLRProj && spriteZoneLRProj > 0.0f && spriteZoneLLProj < 1.0f) {
                        int xLeftStart = clampInt(static_cast<int>(std::round(spriteZoneLLProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);
                        int xLeftEnd = clampInt(static_cast<int>(std::round(spriteZoneLRProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);

                        for (int x = xLeftStart; x <= xLeftEnd; ++x) {
                            float x_prop = (static_cast<float>(x) / (video_.width_ - 1));
                            float x_prop_born = (x_prop - spriteZoneLLProj) / (spriteZoneLRProj - spriteZoneLLProj);

                            float cellwidth = (spriteZoneLRProj - spriteZoneLLProj) / static_cast<float>(spriteZone.width_);

                            int spritezone_idx_horizon = static_cast<int>(static_cast<float>(spriteZone.width_) * x_prop_born);
                            spritezone_idx_horizon = clampInt(spritezone_idx_horizon, 0, spriteZone.width_ - 1);

                            int xcell = static_cast<int>(std::round((spriteZoneLLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                            if (!spritedL[spritezone_idx_vertical][spritezone_idx_horizon] && spriteZone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                                Sprite sprite = spriteZone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                                int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * zref * sprite.scale_ / zMid);
                                int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                                int top = yMid - spriteHeight;
                                int left = xcell - (spriteWidth / 2);

                                for (int y_spr = top; y_spr < yMid; y_spr++) {
                                    float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);

                                    int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);

                                    if (0 <= y_spr && y_spr < video_.height_) {

                                        for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                            if (0 <= x_spr && x_spr < video_.width_) {
                                                float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);

                                                int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);

                                                Color painter = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                                if (painter != Colors::TRANSPARENT)
                                                    video_.buffer_.pixels[x_spr + y_spr * video_.width_] = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                            }
                                        }
                                    }
                                }

                                spritedL[spritezone_idx_vertical][spritezone_idx_horizon] = true;
                            }
                        }
                    }
                    // RIGHT
                    if (spriteZoneRLProj < spriteZoneRRProj && spriteZoneRRProj > 0.0f && spriteZoneRLProj < 1.0f) {
                        int xRightStart = clampInt(static_cast<int>(std::round(spriteZoneRLProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);
                        int xRightEnd = clampInt(static_cast<int>(std::round(spriteZoneRRProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);

                        for (int x = xRightStart; x <= xRightEnd; ++x) {
                            float x_prop = (static_cast<float>(x) / (video_.width_ - 1));
                            float x_prop_born = (x_prop - spriteZoneRLProj) / (spriteZoneRRProj - spriteZoneRLProj);

                            float cellwidth = (spriteZoneRRProj - spriteZoneRLProj) / static_cast<float>(spriteZone.width_);
                            int spritezone_idx_horizon = static_cast<int>(static_cast<float>(spriteZone.width_) * x_prop_born);
                            if (spritezone_idx_horizon >= spriteZone.width_)
                                spritezone_idx_horizon--;
                            if (spritezone_idx_horizon <= 0)
                                spritezone_idx_horizon = 0;

                            int xcell = static_cast<int>(std::round((spriteZoneRLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                            if (!spritedR[spritezone_idx_vertical][spritezone_idx_horizon] && spriteZone.get(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                                Sprite sprite = spriteZone.get(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                                int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * zref * sprite.scale_ / zMid);
                                int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                                int top = yMid - spriteHeight;
                                int left = xcell - (spriteWidth / 2);

                                for (int y_spr = top; y_spr < yMid; y_spr++) {
                                    float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);
                                    int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);

                                    if (0 <= y_spr && y_spr < video_.height_) {
                                        for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                            if (0 <= x_spr && x_spr < video_.width_) {
                                                float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);
                                                int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);

                                                Color painter = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                                if (painter != Colors::TRANSPARENT)
                                                    video_.buffer_.pixels[x_spr + y_spr * video_.width_] = sprite.texture_.get(sprite_texture_idx_horizon, sprite_texture_idx_vertical);
                                            }
                                        }
                                    }
                                }

                                spritedR[spritezone_idx_vertical][spritezone_idx_horizon] = true;
                            }
                        }
                    }
                }
            }

            roadPos += speed * dt;
            roadPos = std::fmod(roadPos, strip_size * 512);

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
            SDL_Delay(static_cast<Uint32>(dt * 100.0f));
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