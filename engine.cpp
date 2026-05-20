#include "camera.hh"
#include "engine.hh"
#include "road.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_events.h>

#define EPSILON 0.00001

namespace fast {
    Engine::Engine(int width, int height, float clipping, float distance) : video_(width, height), sprite_clipping_(clipping), drawing_distance(distance)
    {}

    static uint32_t packRGBA(const Color& c) {
        return (uint32_t(c.r) << 24) |
            (uint32_t(c.g) << 16) |
            (uint32_t(c.b) << 8)  |
            0xFFu;
    }

    void Engine::projection22() {
        std::vector<uint32_t> uploadBuffer(video_.width_ * video_.height_);

        Camera camera{512.0f, video_.height_ / 2.0f, video_.height_ / 2.0f, 500.0f}; 
        camera.heightfol = false;

        Road road{2.0f, camera.distance};
        road.curve_strength = 0.0f;
        road.altitude_strength = 0.0f;

        // CAMERA PROPERTIES
        float cam_height = camera.height_base;
        float horizon = camera.horizon_base;

        // ACCUMULATOR
        float roadPos = 0.0f;
        float smoothedTilt = 0.0f;

        // SKY PARALLAX PARAMETERS
        int delta_horizon = video_.height_ / 8; // to lower the draw zone

        // SKY TEXTURE
#if 0
        std::vector<SkyTexture> skies;
        skies.push_back(SkyTexture::load("textures/clouds.ppm", 0.1f, 0.05f));
        skies.push_back(SkyTexture::load("textures/mountains_back.ppm", 0.2f, 0.15f));
        skies.push_back(SkyTexture::load("textures/mountains.ppm", 0.5f, 0.35f));

        std::vector<std::vector<bool>> spritedL(road.sprite_zone.height_, std::vector<bool>(road.sprite_zone.width_, false));
        std::vector<std::vector<bool>> spritedR(road.sprite_zone.height_, std::vector<bool>(road.sprite_zone.width_, false));
#endif

        float dt = 0.016f;

        // ENGINE & SDL LOOP
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
                        camera.speed_base += 100.0f;
                    else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)
                        camera.speed_base -= 100.0f;
                    else if (event.key.keysym.sym == SDLK_RIGHT)
                        camera.lateral_pivot -= 0.05f;
                    else if (event.key.keysym.sym == SDLK_LEFT)
                        camera.lateral_pivot += 0.05f;
                    else if (event.key.keysym.sym == SDLK_UP)
                        camera.horizon_base += 5.f;
                    else if (event.key.keysym.sym == SDLK_DOWN)
                        camera.horizon_base -= 5.f;
                    else if (event.key.keysym.sym == SDLK_RETURN)
                        camera.distance += 50.f;
                    else if (event.key.keysym.sym == SDLK_BACKSPACE)
                        camera.distance -= 50.f;
                    else if (event.key.keysym.sym == SDLK_z)
                        camera.height_base += 10.0f;
                    else if (event.key.keysym.sym == SDLK_s)
                        camera.height_base -= 10.0f;
                    else if (event.key.keysym.sym == SDLK_q)
                        camera.lateral_pos -= 0.1f;
                    else if (event.key.keysym.sym == SDLK_d)
                        camera.lateral_pos += 0.1f;
                    else if (event.key.keysym.sym == SDLK_o)
                        camera.tilt_strength += 0.1f;
                    else if (event.key.keysym.sym == SDLK_l)
                        camera.tilt_strength -= 0.1f;
                    else if (event.key.keysym.sym == SDLK_y)
                        road.curve_strength += 20.0f;
                    else if (event.key.keysym.sym == SDLK_h)
                        road.curve_strength -= 20.0f;
                    else if (event.key.keysym.sym == SDLK_b)
                        road.altitude_strength += 0.5f;
                    else if (event.key.keysym.sym == SDLK_n)
                        road.altitude_strength -= 0.5f;
                    else if (event.key.keysym.sym == SDLK_t)
                        camera.yawed = !(camera.yawed);
                    else if (event.key.keysym.sym == SDLK_r)
                        camera.latfol = !(camera.latfol);
                    else if (event.key.keysym.sym == SDLK_j) {
                        if (camera.heightfol) {
                            camera.horizon_base = horizon;
                            camera.height_base = cam_height;
                            camera.heightfol = false;
                        }
                        else {
                            camera.height_base = video_.height_ / 2;
                            camera.horizon_base = camera.height_base;
                            camera.heightfol = true;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_c) {
                        camera.lateral_pos = 0.5f;
                        camera.lateral_pivot = 0.5f;
                        camera.height_base = video_.height_ / 2.0f;
                        camera.speed_base = 500.0f;
                        camera.horizon_base = camera.height_base;
                        camera.yawed = true;
                        camera.latfol = true;
                        //camera.heightfol = true;
                        camera.distance = 512.0f;
                        camera.tilt_strength = 3.5f;
                        camera.turn_strength = 0.85f;
                        //road.curve_strength = 200.0f;
                        //road.altitude_strength = 1.0f;
                    }
                }
            }

            // screen normalized camera lateral in road reference
            float playerX = (camera.lateral_pos - 0.5f) * road.width;

            // camera lateral position from track file
            float cameraX = road.trackXAt(roadPos);

            // camera rotation simulation
            float dxAhead = road.trackXAt(roadPos + camera.curve_lookahead) - road.trackXAt(roadPos);
            float camDxDzTarget = camera.turn_strength * (dxAhead / camera.curve_lookahead);
            float camDxDz = camDxDzTarget;

            // compute final x position from the road lateral position, camera lateral position (player),
            // with the rotation effect that simulates the camera looking into the direction of the curve 
            auto camSpaceX = [&](float z) -> float {
                return (road.trackXAt(roadPos + z) - (camera.latfol ? cameraX : 0.0f)) - playerX - (camera.yawed ? (camDxDz * z) : 0.0f);
            };

            // SKY CLEAR
            for (int y = 0; y < video_.height_; ++y) {
                for (int x = 0; x < video_.width_; ++x) {
                    video_.buffer_.pixels[x + y * video_.width_] = Colors::DARK_BLUE;
                }
            }
           

            int laststripIdx = -1;
            
            // ALTITUDE VARIATION OF THE CAMERA (positions are arbitrary values, depends on the rendering you want)
            // DERIVED UNDER CAMERA
            float hac_position = 0 + roadPos;
            float gradient = road.altitudeDerived(hac_position);            
            float targetTilt = gradient * camera.tilt_strength;
            float tiltAlpha = 1.0f - std::exp(-camera.tilt_smoothing * dt);
            smoothedTilt += (targetTilt - smoothedTilt) * tiltAlpha;
            horizon = camera.horizon_base + (camera.heightfol ? (camera.horizon_base * smoothedTilt) : 0.0f);

            // HEIGHT UNDER CAMERA
            float chac_position = camera.distance * 0.75f + roadPos;
            float cam_height_diff = road.altitude(chac_position);
            cam_height = camera.height_base + (camera.heightfol ? cam_height_diff : 0.0f);

            // SPEED PERSPECTIVE ADAPTATION    
            float sac_position = camera.distance + roadPos;        
            float gradient_screen = road.altitudeDerived(sac_position);
            float speed = camera.speed_base - (camera.heightfol ? (camera.speed_base * gradient_screen * 1.1f) : 0.0f);



            // SKY PAINTING
            /*
            for (size_t i = 0; i < skies.size(); i++) {
                float skyBottom = horizon + delta_horizon;
                float skyBottomBase = camera.horizon_base + delta_horizon;
                float normalized_under_horizon_base = 1 - (skyBottomBase / static_cast<float>(video_.height_));
                float verticalShift = ((skyBottomBase - skyBottom) / static_cast<float>(video_.height_)) * skies[i].verticalShiftStrength_;

                int skyYEnd = std::min(static_cast<int>(std::floor(skyBottom)), video_.height_ - 1);
                if (skyYEnd < 0)
                    continue;

                for (int y = skyYEnd; y >= 0; y--) {
                    float y_prop = static_cast<float>(y) / static_cast<float>(video_.height_ - 1);
                    float y_prop_shifted = y_prop + normalized_under_horizon_base + verticalShift;
                    
                    int sky_idx_vertical = static_cast<int>(std::floor(y_prop_shifted * static_cast<float>(skies[i].height_ - 1))); 
                    sky_idx_vertical = std::clamp(sky_idx_vertical, 0, skies[i].height_ - 1);

                    for (int x = 0; x < video_.width_; x++) {
                        float x_prop = static_cast<float>(x) / static_cast<float>(video_.width_ - 1);

                        float x_prop_shifted = x_prop + (camDxDz * 1000.0f * skies[i].horizontalShiftStrength_);
                        if (x_prop_shifted < 0.0f) x_prop_shifted += 1.0f;
                        x_prop_shifted = std::fmod(x_prop_shifted, 1.0f);
                        
                        int sky_idx_horizon = static_cast<int>(std::floor(x_prop_shifted * static_cast<float>(skies[i].width_ - 1))); 
                        sky_idx_horizon = std::clamp(sky_idx_horizon, 0, skies[i].width_ - 1);

                        Color painter = skies[i].get(sky_idx_horizon, sky_idx_vertical);
                        if (painter != Colors::TRANSPARENT)
                            video_.buffer_.pixels[x + y * video_.width_] = skies[i].get(sky_idx_horizon, sky_idx_vertical);
                    }
                }
            }
            */


            float cameraOffset = std::fmod(roadPos, road.segment_length);
            if (cameraOffset < 0.0f)
                cameraOffset += road.segment_length;

            // PROJECTION LOOP
            for (int i = this->drawing_distance; i >= 1; --i) {
                float zFar = (i * road.segment_length) - cameraOffset;
                float zNear = ((i - 1) * road.segment_length) - cameraOffset;

                if (zNear < 2.0f)
                    zNear = 2.0f;
                if (zFar <= zNear)
                    continue;

                // y_screen bounds computation
                float screenYFar = horizon + (camera.distance * (cam_height - road.altitude(zFar + roadPos))) / zFar;
                float screenYNear = horizon + (camera.distance * (cam_height - road.altitude(zNear + roadPos))) / zNear;

                int yTop = static_cast<int>(std::round(screenYFar));
                int yBottom = static_cast<int>(std::round(screenYNear));

                if (yTop > yBottom)
                    continue;

                if (yBottom < 0 || yTop >= video_.height_)
                    continue;

                int yStart = std::clamp(yTop, 0, video_.height_ - 1);
                int yEnd = std::clamp(yBottom, 0, video_.height_ - 1);

                if (yEnd < yStart)
                     continue;

                // trapezoidal apexes computation, they represent the road portion to draw in the interpollation part (one segment)
                float xFar = camSpaceX(zFar);
                float xNear = camSpaceX(zNear);

                float centerFarProj = camera.lateral_pivot + (xFar * road.zref) / zFar;
                float centerNearProj = camera.lateral_pivot + (xNear * road.zref) / zNear;

                float roadWidthFarProj = (road.width * road.zref) / zFar;
                float roadWidthNearProj = (road.width * road.zref) / zNear;

                float roadLeftFarProj = centerFarProj - (roadWidthFarProj / 2.0f);
                float roadRightFarProj = centerFarProj + (roadWidthFarProj / 2.0f);
                float roadLeftNearProj = centerNearProj - (roadWidthNearProj / 2.0f);
                float roadRightNearProj = centerNearProj + (roadWidthNearProj / 2.0f);

                // SpriteZone flags reset
                int stripIdx = static_cast<int>((zFar + roadPos) / road.strip_size);
                if (stripIdx != laststripIdx) {
                    laststripIdx = stripIdx;
#if 0
                    for (auto& row : spritedL) std::fill(row.begin(), row.end(), false);
                    for (auto& row : spritedR) std::fill(row.begin(), row.end(), false);
#endif

                }

                // INTERPOLATION
                for (int y = yStart; y <= yEnd; y++) {
                    // normalized vertical coordinate of the trapeze
                    float y_prop = yEnd == yStart ? 0.5f : (static_cast<float>(y) - screenYFar) / (screenYNear - screenYFar);

                    if (y_prop < 0.0f) y_prop = 0.0f;
                    if (y_prop > 1.0f) y_prop = 1.0f;

                    // road borders recalculated for each line in order to form the lateral sides of the trapeze
                    float leftProj  = roadLeftFarProj + (roadLeftNearProj - roadLeftFarProj) * y_prop;
                    float rightProj = roadRightFarProj + (roadRightNearProj - roadRightFarProj) * y_prop;

                    float projWidth = rightProj - leftProj;
                    if (projWidth <= EPSILON)
                        continue;

                    // normalized depth computation in one strip for texture vertical indexes
                    float invZFar = 1.0f / zFar;
                    float invZNear = 1.0f / zNear;
                    float invZ = invZFar + (invZNear - invZFar) * y_prop;
                    if (invZ <= EPSILON)
                        continue;

                    float zSample = 1.0f / invZ;
                    float depthPos = std::fmod(zSample + roadPos, road.strip_size);
                    if (depthPos < 0.0f)
                        depthPos += road.strip_size;

                    float depth_prop = depthPos / road.strip_size;

                    // GRASS PAINTING
                    bool alternateBand = static_cast<int>((zSample + roadPos) / (road.strip_size * 0.5f)) % 2 == 0;
                    Color grassColor = alternateBand ? Colors::JADE : Colors::JADE2;
                    Color roadColor = alternateBand ? Colors::BLACK : Colors::DARK_GRAY;

                    auto paintGrass = [&](int x, float x_prop) {
#if 0
                        int grass_idx_horizon = static_cast<int>(std::floor((std::fmod(x_prop, grassWidthProj) / grassWidthProj) * static_cast<float>(road.grass_texture.width_)));
                        grass_idx_horizon = std::clamp(grass_idx_horizon, 0, road.grass_texture.width_ - 1);

                        video_.buffer_.pixels[x + y * video_.width_] = road.grass_texture.get(grass_idx_horizon, grass_idx_vertical);
#endif
                        video_.buffer_.pixels[x + y * video_.width_] = grassColor;
                    };

                    if (rightProj <= 0.0f) {
                        for (int x = 0; x < video_.width_; x++) {
                            float x_prop = (static_cast<float>(x) / static_cast<float>(video_.width_ - 1)) - rightProj;
                            paintGrass(x, x_prop);
                        }

                        continue;
                    }

                    if (leftProj >= 1.0f) {
                        for (int x = 0; x < video_.width_; x++) {
                            float x_prop = leftProj - (static_cast<float>(x) / static_cast<float>(video_.width_ - 1));
                            paintGrass(x, x_prop);
                        }

                        continue;
                    }

                    int leftPix = static_cast<int>(std::round(leftProj * static_cast<float>(video_.width_ - 1)));
                    int rightPix = static_cast<int>(std::round(rightProj * static_cast<float>(video_.width_ - 1)));

                    if (leftPix > rightPix)
                        std::swap(leftPix, rightPix);

                    // road border in pixels
                    int xStart = std::clamp(leftPix, 0, video_.width_ - 1);
                    int xEnd = std::clamp(rightPix, 0, video_.width_ - 1);

                    if (xEnd < xStart)
                        continue;

                    for (int x = xStart - 1; x >= 0; x--) {
                        // screen normalized x from left border of the road (x_prop = 0 <=> leftProj; x_prop = 1 <=> leftProj - 1)
                        float x_prop = leftProj - (static_cast<float>(x) / static_cast<float>(video_.width_ - 1));

                        paintGrass(x, x_prop);
                    }

                    for (int x = xEnd + 1; x < video_.width_; x++) {
                        // screen normalized x from right border of the road (x_prop = 0 <=> rightProj; x_prop = 1 <=> rightProj + 1) 
                        float x_prop = (static_cast<float>(x) / static_cast<float>(video_.width_ - 1)) - rightProj;

                        paintGrass(x, x_prop);
                    }

                   
                    // ROAD PAINTING
#if 0
                    int road_idx_vertical = static_cast<int>(depth_prop * static_cast<float>(road.road_texture.height_));
                    road_idx_vertical = std::clamp(road_idx_vertical, 0, road.road_texture.height_ - 1);
#endif

                    for (int x = xStart; x <= xEnd; x++) {
                        // normalized horizontal coordinate of the road
                        float x_prop = ((static_cast<float>(x) / (video_.width_ - 1)) - leftProj) / projWidth;

                        if (x_prop < 0.0f) x_prop = 0.0f;
                        if (x_prop > 1.0f) x_prop = 1.0f;

#if 0
                        int road_idx_horizon = static_cast<int>(x_prop * static_cast<float>(road.road_texture.width_));
                        road_idx_horizon = std::clamp(road_idx_horizon, 0, road.road_texture.width_ - 1);

                        if (0 <= x && x < video_.width_ && 0 <= y && y < video_.height_)
                            video_.buffer_.pixels[x + y * video_.width_] = road.road_texture.get(road_idx_horizon, road_idx_vertical);
#endif

                        if (0 <= x && x < video_.width_ && 0 <= y && y < video_.height_)
                            video_.buffer_.pixels[x + y * video_.width_] = roadColor;

                    }
                }

                // SPRITE ZONE
#if 0
                if (i < this->drawing_distance / this->sprite_clipping_) {
                    float zMid = 2 / ((1.0f / zFar) + (1.0f / zNear));

                    float screenYMid = horizon + (camera.distance * (cam_height - road.altitude(zMid + roadPos))) / zMid;

                    int segmentIdx = static_cast<int>(std::floor(std::fmod(zMid + roadPos, road.strip_size) / road.segment_length));

                    int yMid = static_cast<int>(std::round(screenYMid));
                    yMid = std::clamp(yMid, 0, video_.height_ + video_.height_ / 4);

                    float xMid = camSpaceX(zMid);

                    float spriteZoneLRProj = camera.lateral_pivot + ((xMid - (road.width * 0.5f) - road.sprite_zone.roadOffset_) * road.zref) / zMid; // right edge of left zone
                    float spriteZoneLLProj = camera.lateral_pivot + ((xMid - (road.width * 0.5f) - road.sprite_zone.roadOffset_ - road.sprite_zone.widthScale_) * road.zref) / zMid; // left edge of left zone
                    float spriteZoneRLProj = camera.lateral_pivot + ((xMid + (road.width * 0.5f) + road.sprite_zone.roadOffset_) * road.zref) / zMid; // left edge of right zone
                    float spriteZoneRRProj = camera.lateral_pivot + ((xMid + (road.width * 0.5f) + road.sprite_zone.roadOffset_ + road.sprite_zone.widthScale_) * road.zref) / zMid; // right edge of right zone

                    int spritezone_idx_vertical = segmentIdx;
                    spritezone_idx_vertical = std::clamp(spritezone_idx_vertical, 0, road.sprite_zone.height_ - 1);

                    // LEFT
                    if (spriteZoneLLProj < spriteZoneLRProj && spriteZoneLRProj > 0.0f && spriteZoneLLProj < 1.0f) {
                        int xLeftStart = std::clamp(static_cast<int>(std::round(spriteZoneLLProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);
                        int xLeftEnd = std::clamp(static_cast<int>(std::round(spriteZoneLRProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);

                        for (int x = xLeftStart; x <= xLeftEnd; ++x) {
                            float x_prop = (static_cast<float>(x) / (video_.width_ - 1));
                            float x_prop_born = (x_prop - spriteZoneLLProj) / (spriteZoneLRProj - spriteZoneLLProj);

                            float cellwidth = (spriteZoneLRProj - spriteZoneLLProj) / static_cast<float>(road.sprite_zone.width_);

                            int spritezone_idx_horizon = static_cast<int>(static_cast<float>(road.sprite_zone.width_) * x_prop_born);
                            spritezone_idx_horizon = std::clamp(spritezone_idx_horizon, 0, road.sprite_zone.width_ - 1);

                            int xcell = static_cast<int>(std::round((spriteZoneLLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                            if (!spritedL[spritezone_idx_vertical][spritezone_idx_horizon] && road.sprite_zone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                                Sprite sprite = road.sprite_zone.getFromLeft(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                                int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * road.zref * sprite.scale_ / zMid);
                                int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                                int top = yMid - spriteHeight;
                                int left = xcell - (spriteWidth / 2);

                                for (int y_spr = top; y_spr < yMid; y_spr++) {
                                    float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);

                                    int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);
                                    sprite_texture_idx_vertical = std::clamp(sprite_texture_idx_vertical, 0, sprite.texture_.height_ - 1);

                                    if (0 <= y_spr && y_spr < video_.height_) {

                                        for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                            if (0 <= x_spr && x_spr < video_.width_) {
                                                float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);

                                                int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);
                                                sprite_texture_idx_horizon = std::clamp(sprite_texture_idx_horizon, 0, sprite.texture_.width_ - 1);

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
                        int xRightStart = std::clamp(static_cast<int>(std::round(spriteZoneRLProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);
                        int xRightEnd = std::clamp(static_cast<int>(std::round(spriteZoneRRProj * static_cast<float>(video_.width_ - 1))), 0, video_.width_ - 1);

                        for (int x = xRightStart; x <= xRightEnd; ++x) {
                            float x_prop = (static_cast<float>(x) / (video_.width_ - 1));
                            float x_prop_born = (x_prop - spriteZoneRLProj) / (spriteZoneRRProj - spriteZoneRLProj);

                            float cellwidth = (spriteZoneRRProj - spriteZoneRLProj) / static_cast<float>(road.sprite_zone.width_);

                            int spritezone_idx_horizon = static_cast<int>(static_cast<float>(road.sprite_zone.width_) * x_prop_born);
                            spritezone_idx_horizon = std::clamp(spritezone_idx_horizon, 0, road.sprite_zone.width_ - 1);

                            int xcell = static_cast<int>(std::round((spriteZoneRLProj + (spritezone_idx_horizon * cellwidth) + (cellwidth / 2)) * static_cast<float>(video_.width_)));

                            if (!spritedR[spritezone_idx_vertical][spritezone_idx_horizon] && road.sprite_zone.get(spritezone_idx_horizon, spritezone_idx_vertical).active) {
                                Sprite sprite = road.sprite_zone.get(spritezone_idx_horizon, spritezone_idx_vertical).sprite_;

                                int spriteWidth = static_cast<int>(static_cast<float>(video_.width_) * road.zref * sprite.scale_ / zMid);
                                int spriteHeight = spriteWidth * sprite.texture_.height_ / sprite.texture_.width_;

                                int top = yMid - spriteHeight;
                                int left = xcell - (spriteWidth / 2);

                                for (int y_spr = top; y_spr < yMid; y_spr++) {
                                    float y_spr_normalized = static_cast<float>(y_spr - top) / static_cast<float>(spriteHeight);

                                    int sprite_texture_idx_vertical = static_cast<int>(static_cast<float>(sprite.texture_.height_) * y_spr_normalized);
                                    sprite_texture_idx_vertical = std::clamp(sprite_texture_idx_vertical, 0, sprite.texture_.height_ - 1);

                                    if (0 <= y_spr && y_spr < video_.height_) {
                                        for (int x_spr = left; x_spr < xcell + (spriteWidth / 2); x_spr++) {
                                            if (0 <= x_spr && x_spr < video_.width_) {
                                                float x_spr_normalized = static_cast<float>(x_spr - left) / static_cast<float>(spriteWidth);

                                                int sprite_texture_idx_horizon = static_cast<int>(static_cast<float>(sprite.texture_.width_) * x_spr_normalized);
                                                sprite_texture_idx_horizon = std::clamp(sprite_texture_idx_horizon, 0, sprite.texture_.width_ - 1);

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
#endif
            }

            roadPos += speed * dt;
            roadPos = std::fmod(roadPos, road.altitude_step * road.altitudes.size());

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
}
