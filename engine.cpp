#include "engine.hh"
#include "texture.hh"

#include <cmath>
#include <cstdint>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>

#define EPSILON 0.00001

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

    std::vector<float> parseAltitudeFile(const std::string& filename) {
        std::vector<float> altitudes;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Erreur: Impossible d'ouvrir le fichier " << filename << std::endl;
            return altitudes;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            try {
                float value = std::stof(line);
                altitudes.push_back(value);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Erreur: Ligne non numérique ignorée: " << line << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "Erreur: Valeur hors limites ignorée: " << line << std::endl;
            }
        }
        
        file.close();
        return altitudes;
    }

    struct TrackProfile {
        std::vector<float> altitude;
        std::vector<float> centerOffset; // normalized screen space offset (0 = centered)
    };

    static TrackProfile parseTrackProfileFile(const std::string& filename) {
        TrackProfile profile;
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Erreur: Impossible d'ouvrir le fichier " << filename << std::endl;
            return profile;
        }

        enum class TrackMode { Offset, Curvature };
        TrackMode mode = TrackMode::Offset;

        std::string line;
        float curvatureAccum = 0.0f;
        while (std::getline(file, line)) {
            // Format:
            //   mode offset
            //   <altitude> <centerOffset>
            //
            // or
            //   mode curvature
            //   <altitude> <curvature>
            //
            // centerOffset is a normalized horizontal offset (e.g. -0.25..0.25)
            // curvature is an incremental steering value integrated over the track
            try {
                // skip leading spaces
                size_t start = 0;
                while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
                    ++start;
                if (start >= line.size())
                    continue;
                if (line[start] == '#')
                    continue;

                // optional mode header
                if (line.compare(start, 4, "mode") == 0) {
                    if (line.find("curvature", start) != std::string::npos)
                        mode = TrackMode::Curvature;
                    else if (line.find("offset", start) != std::string::npos)
                        mode = TrackMode::Offset;
                    continue;
                }

                size_t pos = start;
                float a = std::stof(line.substr(pos), &pos);
                profile.altitude.push_back(a);

                // advance pos within original line
                pos += start;
                while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
                    ++pos;

                float second = 0.0f;
                if (pos < line.size())
                    second = std::stof(line.substr(pos));

                if (mode == TrackMode::Offset) {
                    profile.centerOffset.push_back(second);
                } else {
                    curvatureAccum += second;
                    profile.centerOffset.push_back(curvatureAccum);
                }
            } catch (const std::exception&) {
                // ignore malformed lines (keeps file flexible for comments/blanks)
            }
        }

        if (profile.centerOffset.size() != profile.altitude.size()) {
            profile.centerOffset.resize(profile.altitude.size(), 0.0f);
        }

        return profile;
    }

    float altitude_sin(float z, float a, float f, float offset) {
        return a * std::sin(f * z + offset);
    }

    float altitude_derived_sin(float z, float a, float f, float offset) {
        return a * f * std::cos(f * z + offset);
    }

    float altitude(const std::vector<float>& altitudes, float t, int i, float tens) {
        i = i % altitudes.size();
        int im1 = i - 1;
        if (im1 < 0) im1 = altitudes.size() - 1; 
        int ip1 = (i + 1) % altitudes.size();
        int ip2 = (i + 2) % altitudes.size();

        return ((altitudes[i])
                        + (-tens * altitudes[im1] + tens * altitudes[ip1]) * t
                        + (2 * tens * altitudes[im1] + (tens - 3) * altitudes[i] + (3 - 2 * tens) * altitudes[ip1] - tens * altitudes[ip2]) * std::pow(t, 2)
                        + (-tens * altitudes[im1] + (2 - tens) * altitudes[i] + (tens - 2) * altitudes[ip1] + tens * altitudes[ip2]) * std::pow(t, 3));

    }

    float altitude_derived(const std::vector<float>& altitudes, float t, int i, float tens) {
        i = i % altitudes.size();
        int im1 = i - 1;
        if (im1 < 0) im1 = altitudes.size() - 1; 
        int ip1 = (i + 1) % altitudes.size();
        int ip2 = (i + 2) % altitudes.size();

        return ((-tens * altitudes[im1] + tens * altitudes[ip1])
                        + (2 * tens * altitudes[im1] + (tens - 3) * altitudes[i] + (3 - 2 * tens) * altitudes[ip1] - tens * altitudes[ip2]) * 2 * t
                        + (-tens * altitudes[im1] + (2 - tens) * altitudes[i] + (tens - 2) * altitudes[ip1] + tens * altitudes[ip2]) * 3 * std::pow(t, 2));

    }

    static float splineValue(const std::vector<float>& v, float t, int i, float tens) {
        // Same Catmull-Rom style interpolation as altitude()
        i = i % v.size();
        int im1 = i - 1;
        if (im1 < 0) im1 = static_cast<int>(v.size()) - 1;
        int ip1 = (i + 1) % static_cast<int>(v.size());
        int ip2 = (i + 2) % static_cast<int>(v.size());

        return (v[i]
            + (-tens * v[im1] + tens * v[ip1]) * t
            + (2 * tens * v[im1] + (tens - 3) * v[i] + (3 - 2 * tens) * v[ip1] - tens * v[ip2]) * std::pow(t, 2)
            + (-tens * v[im1] + (2 - tens) * v[i] + (tens - 2) * v[ip1] + tens * v[ip2]) * std::pow(t, 3));
    }

    void Engine::projection2() {
        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        // CAMERA PARAMETERS
        float distance = 512.0f; // between camera & screen
        float cam_height = video_.height_ / 2.0f; // between ground & camera
        float cam_height_base = video_.height_ / 2.0f; // between ground & camera
        float horizon = video_.height_ / 2.0f;
        float horizon_base = video_.height_ / 2.0f;
        float pivot = 0.5f; // screen-space pivot (used by sprite-zone projection)

        float strip_size = 512.0f;
        float speed_base = 500.0f;
        float dt = 0.016f;

        // ACCUMULATOR
        float roadPos = 0.0f;
        float camDxDzSmoothed = 0.0f; // camera yaw state (smoothed)

        // ROAD PARAMETERS
        float roadWidth = 1.5f; // road width scale
        float lateral_pos = 0.5f;
        float zref = distance;

        // CURVE PARAMETERS
        // With pseudo-3D curves, it's much more stable to keep "curve" in world X units
        // and project (roadX - cameraX) / z onto screen.
        // IMPORTANT: keep X in the SAME normalized units as roadWidth/sprite offsets.
        // If this is too large, everything flies off-screen (including sprite zones).
        float curveWorldScale = 180.0f; // amplifies centerOffset/curvature integration (tune this)

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

        // HILL PARAMETERS (legacy sinus values kept in commented block below)
        TrackProfile track = parseTrackProfileFile("roads/track.txt");
        if (track.altitude.empty()) {
            // Backward compatible fallback
            track.altitude = parseAltitudeFile("roads/road.txt");
            track.centerOffset.assign(track.altitude.size(), 0.0f);
        }
        std::cout << "track size = " << track.altitude.size() << std::endl;
        float altitudeStep = strip_size * 2.0f;
        float tension = 0.5f;

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
                        speed_base += 100.0f;
                    else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)
                        speed_base -= 100.0f;
                    else if (event.key.keysym.sym == SDLK_RIGHT)
                        pivot -= 0.05f;
                    else if (event.key.keysym.sym == SDLK_LEFT)
                        pivot += 0.05f;
                    else if (event.key.keysym.sym == SDLK_UP)
                        horizon_base += 5.f;
                    else if (event.key.keysym.sym == SDLK_DOWN)
                        horizon_base -= 5.f;
                    else if (event.key.keysym.sym == SDLK_z)
                        cam_height_base += 10.0f;
                    else if (event.key.keysym.sym == SDLK_s)
                        cam_height_base -= 10.0f;
                    else if (event.key.keysym.sym == SDLK_q)
                        lateral_pos -= 0.1f;
                    else if (event.key.keysym.sym == SDLK_d)
                        lateral_pos += 0.1f;
                    else if (event.key.keysym.sym == SDLK_t) {
                        horizon_base += 5.0f;
                        cam_height_base -= 5.0f;
                    }
                    else if (event.key.keysym.sym == SDLK_g) {
                        horizon_base -= 5.0f;
                        cam_height_base += 5.0f;
                    }
                    else if (event.key.keysym.sym == SDLK_c) {
                        lateral_pos = 0.5f;
                        pivot = 0.5f;
                        cam_height_base = video_.height_ / 2.0f;
                        speed_base = 500.0f;
                        horizon_base = cam_height_base;
                    }
                }
            }

            // TRACK X IN WORLD UNITS (curve integrated over distance)
            auto trackXAt = [&](float worldZ) -> float {
                if (track.centerOffset.empty())
                    return 0.0f;
                int idx = static_cast<int>(worldZ / altitudeStep);
                float t = std::fmod(worldZ, altitudeStep) / altitudeStep;
                if (t < 0.0f) t += 1.0f;
                return curveWorldScale * splineValue(track.centerOffset, t, idx, tension);
            };

            // camera lateral in road reference
            float playerX = (lateral_pos - 0.5f) * roadWidth;

            // Camera X follows track (car attached to camera)
            float cameraX = trackXAt(roadPos);

            // Camera yaw (look into the curve):
            // approximate heading by looking ahead on the track centerline.
            float lookAheadZ = distance;
            float cameraTurnStrength = 0.85f; // base strength
            float cameraTurnSmoothing = 0.08f;
            float maxCamDxDz = 0.60f; // clamp (prevents too aggressive rotation)
            float dxAhead = trackXAt(roadPos + lookAheadZ) - trackXAt(roadPos);
            float camDxDzTarget = cameraTurnStrength * (dxAhead / lookAheadZ);
            if (camDxDzTarget > maxCamDxDz) camDxDzTarget = maxCamDxDz;
            if (camDxDzTarget < -maxCamDxDz) camDxDzTarget = -maxCamDxDz;
            camDxDzSmoothed += (camDxDzTarget - camDxDzSmoothed) * cameraTurnSmoothing;
            float camDxDz = camDxDzTarget;

            auto camSpaceX = [&](float z) -> float {
                // Convert world road center at (roadPos+z) into camera space with yaw.
                // yaw term (camDxDz*z) makes the camera look towards the curve direction.
                return (trackXAt(roadPos + z) - cameraX) - playerX - (camDxDz * z);
            };

            // Near segment (for sprite zones borders) - project center and width at z=distance
            float xNear = camSpaceX(distance);
            float roadCenterNearProj = pivot + (xNear * zref) / distance;
            float halfRoadNearProj = ((roadWidth * 0.5f) * zref) / distance;
            float roadLeft = roadCenterNearProj - halfRoadNearProj;
            float roadRight = roadCenterNearProj + halfRoadNearProj;

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
            
            float tiltstr = 2.8f;

            // *** OLD SINUS ALTITUDE CALCULATION METHOD*** //

            // // DERIVED UNDER CAMERA
            // float gradient = altitude_derived_sin(0 + roadPos, amplitude, freq, phase);
            // //std::cout << gradient << '\n';
            // float tilt = gradient * tiltstr;
            // horizon = horizon_base + (horizon_base * (tilt));

            // // HEIGHT UNDER CAMERA
            // float cam_height_diff = altitude_sin(distance * 0.75f + roadPos, amplitude, freq, phase);
            // std::cout << cam_height_diff << '\n';
            // cam_height = cam_height_base + cam_height_diff;

            // // SPEED PERSPECTIVE ADAPTATION
            // float gradient_screen = altitude_derived_sin(distance + roadPos, amplitude, freq, phase);
            // float speed = speed_base - speed_base * gradient_screen * 1.1f;

            // *** END OF OLD SINUS ALTITUDE CALCULATION METHOD *** //

            

            // *** NEW PARSING ALTITUDE CALCULATION METHOD *** //

            // DERIVED UNDER CAMERA
            int stripIdxPrime = (0 + roadPos) / altitudeStep;
            float stripPosPrime = std::fmod(0 + roadPos, altitudeStep) / altitudeStep;
            float gradient = altitude_derived(track.altitude, stripPosPrime, stripIdxPrime, tension) / altitudeStep;            
            float tilt = gradient * tiltstr;
            std::cout << stripIdxPrime << '\n';
            horizon = horizon_base + (horizon_base * (tilt));

            // HEIGHT UNDER CAMERA
            int stripIdx = (distance * 0.75f + roadPos) / altitudeStep;
            float stripPos = std::fmod(distance * 0.75f + roadPos, altitudeStep) / altitudeStep;
            float cam_height_diff = altitude(track.altitude, stripPos, stripIdx, tension);
            cam_height = cam_height_base + cam_height_diff;
            std::cout << cam_height << '\n';

            // SPEED PERSPECTIVE ADAPTATION            
            int stripIdxPrimeSpeed = (distance + roadPos) / altitudeStep;
            float stripPosPrimeSpeed = std::fmod(distance + roadPos, altitudeStep) / altitudeStep;
            float gradient_screen = altitude_derived(track.altitude, stripPosPrimeSpeed, stripIdxPrimeSpeed, tension) / altitudeStep;
            float speed = speed_base /*- speed_base * gradient_screen * 1.1f*/;
            (void)gradient_screen;

            // *** END OF NEW PARSING ALTITUDE CALCULATION METHOD *** //


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
                int stripIdxZFar = (zFar + roadPos) / altitudeStep;
                float stripPosZFar = std::fmod(zFar + roadPos, altitudeStep) / altitudeStep;
                int stripIdxZNear = (zNear + roadPos) / altitudeStep;
                float stripPosZNear = std::fmod(zNear + roadPos, altitudeStep) / altitudeStep;

                float screenYFar = horizon + (distance * (cam_height - altitude(track.altitude, stripPosZFar, stripIdxZFar, tension))) / zFar;
                float screenYNear = horizon + (distance * (cam_height - altitude(track.altitude, stripPosZNear, stripIdxZNear, tension))) / zNear;

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

                // Road borders with real pseudo-3D curve projection
                float xFar = camSpaceX(zFar);
                float xNear = camSpaceX(zNear);

                float centerFarProj = pivot + (xFar * zref) / zFar;
                float centerNearProj = pivot + (xNear * zref) / zNear;

                float halfRoadFarProj = ((roadWidth * 0.5f) * zref) / zFar;
                float halfRoadNearProj = ((roadWidth * 0.5f) * zref) / zNear;

                float roadLeftFarProj = centerFarProj - halfRoadFarProj;
                float roadRightFarProj = centerFarProj + halfRoadFarProj;
                float roadLeftNearProj = centerNearProj - halfRoadNearProj;
                float roadRightNearProj = centerNearProj + halfRoadNearProj;

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
                    if (projWidth <= EPSILON) {
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
                    if (invZ <= EPSILON)
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
                if (i < drawDistance / 10) {
                    float zMid = 2 / ((1.0f / zFar) + (1.0f / zNear));

                    int stripIdxZMid = (zMid + roadPos) / altitudeStep;
                    float stripPosZMid = std::fmod(zMid + roadPos, altitudeStep) / altitudeStep;

                    float screenYMid = horizon + (distance * (cam_height - altitude(track.altitude, stripPosZMid, stripIdxZMid, tension))) / zMid;

                    int segmentIdx = static_cast<int>(std::floor(std::fmod(zMid + roadPos, strip_size) / segmentLength));

                    int yMid = static_cast<int>(std::round(screenYMid));
                    yMid = clampInt(yMid, 0, video_.height_ - 1);

                    float xMid = camSpaceX(zMid);

                    float spriteZoneLRProj = pivot + ((xMid - (roadWidth * 0.5f) - spriteZone.roadOffset_) * zref) / zMid; // right edge of left zone
                    float spriteZoneLLProj = pivot + ((xMid - (roadWidth * 0.5f) - spriteZone.roadOffset_ - spriteZone.widthScale_) * zref) / zMid; // left edge of left zone
                    float spriteZoneRLProj = pivot + ((xMid + (roadWidth * 0.5f) + spriteZone.roadOffset_) * zref) / zMid; // left edge of right zone
                    float spriteZoneRRProj = pivot + ((xMid + (roadWidth * 0.5f) + spriteZone.roadOffset_ + spriteZone.widthScale_) * zref) / zMid; // right edge of right zone

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
            roadPos = std::fmod(roadPos, altitudeStep * track.altitude.size());

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
