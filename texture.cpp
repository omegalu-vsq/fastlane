#include "texture.hh"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace fast {

    Texture::Texture(int width, int height) : width_(width), height_(height) {
        pixels_.resize(height);

        for (int y = 0; y < height; y++)
            pixels_[y].resize(width);
    }

    Color get_color_from_char(char c) {
        switch (std::toupper(c)) {
            case 'R': return Colors::RED;
            case 'G': return Colors::GREEN;
            case 'B': return Colors::BLUE;
            case 'W': return Colors::WHITE;
            case 'K': return Colors::BLACK;
            case 'J': return Colors::JADE;
            case 'D': return Colors::DARK_BLUE;
            case 'Y': return Colors::GRAY;
            case '.': return Colors::BLACK;
            case '-': return Colors::TRANSPARENT;
            default:
                throw std::runtime_error("/!\\ Texture error: Unknown color character: " + c);
        }
    }

    SpriteSlot get_sprite_from_char(char c) {
        SpriteSlot slot;
        switch (std::toupper(c)) {
            case 'R':
                slot.active = true;
                slot.sprite_ = Sprite("textures/rom_transparent.ppm", 0.3f);
                break;
            case 'S':
                slot.active = true;
                slot.sprite_ = Sprite("textures/sunflower.ppm", 0.1f);
                break;
            case '.':
                slot.active = false;
                break;
            default:
                throw std::runtime_error("/!\\ SpriteZone error: Unknown sprite character: " + std::string(1, c));
        }
        return slot;
    }

    Texture Texture::load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("/!\\ Texture error: File openning failed. (" + filename + ")");

        std::string mode;
        std::getline(file, mode);
        
        mode.erase(0, mode.find_first_not_of(" \t\r\n"));
        mode.erase(mode.find_last_not_of(" \t\r\n") + 1);

        int width, height;
        file >> width >> height;
        file.ignore();

        Texture texture(width, height);

        if (mode == "PALETTE") {
            for (int y = 0; y < height; y++) {
                std::string line;
                std::getline(file, line);
                
                if (line.length() < static_cast<size_t>(width)) {
                    throw std::runtime_error("/!\\ Texture error: line " + std::to_string(y) + 
                                           " is too short (expected " + std::to_string(width) + 
                                           " characters, got " + std::to_string(line.length()) + ")");
                }

                for (int x = 0; x < width; x++)
                    texture.pixels_[y][x] = get_color_from_char(line[x]);
            }
        } 
        else if (mode == "RGB" || mode == "P3") {
            if (mode == "P3") {
                int maxval;
                file >> maxval;
                file.ignore();
            }

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int r, g, b;
                    file >> r >> g >> b;

                    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
                        throw std::runtime_error("/!\\ Texture error: RGB values must be between 0 and 255");

                    texture.pixels_[y][x] = Color(
                        static_cast<uint8_t>(r),
                        static_cast<uint8_t>(g),
                        static_cast<uint8_t>(b)
                    );
                }
            }
        }
        else {
            throw std::runtime_error("/!\\ Unknown texture mode, must be PALETTE, RGB or P3");
        }

        return texture;
    }

    Color Texture::get(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            throw std::out_of_range("/!\\ Texture coordinates out of range");

        return pixels_[y][x];
    }

    void Texture::flip_horizontal() {
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_ / 2; x++) {
                Color temp = pixels_[y][x];
                pixels_[y][x] = pixels_[y][width_ - 1 - x];
                pixels_[y][width_ - 1 - x] = temp;
            }
        }
    }

    void Texture::flip_vertical() {
        for (int y = 0; y < height_ / 2; y++) {
            for (int x = 0; x < width_; x++) {
                Color temp = pixels_[y][x];
                pixels_[y][x] = pixels_[height_ - 1 - y][x];
                pixels_[height_ - 1 - y][x] = temp;
            }
        }
    }

    Sprite::Sprite(const std::string& texture, float scale) : scale_(scale) {
        texture_ = Texture::load(texture);
    }

    SpriteZone::SpriteZone(int width, int height) : width_(width), height_(height) {
        zone_.resize(height);

        for (int y = 0; y < height; y++)
            zone_[y].resize(width);
    }

    SpriteZone SpriteZone::load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open())
            throw std::runtime_error("/!\\ SpriteZone error: File openning failed. (" + filename + ")");

        std::string mode;
        std::getline(file, mode);
        
        mode.erase(0, mode.find_first_not_of(" \t\r\n"));
        mode.erase(mode.find_last_not_of(" \t\r\n") + 1);

        int width, height;
        file >> width >> height;
        file.ignore();

        SpriteZone spritezone(width, height);

        if (mode == "PALETTE") {
            for (int y = 0; y < height; y++) {
                std::string line;
                std::getline(file, line);
                
                if (line.length() < static_cast<size_t>(width)) {
                    throw std::runtime_error("/!\\ SpriteZone error: line " + std::to_string(y) + 
                                           " is too short (expected " + std::to_string(width) + 
                                           " characters, got " + std::to_string(line.length()) + ")");
                }

                for (int x = 0; x < width; x++)
                    spritezone.zone_[y][x] = get_sprite_from_char(line[x]);
            }
        }
        else {
            throw std::runtime_error("/!\\ Unknown SpriteZone mode, must be PALETTE");
        }

        return spritezone;
    }


    SpriteSlot SpriteZone::get(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            throw std::out_of_range("/!\\ SpriteZone coordinates out of range");

        return zone_[y][x];
    }

    SpriteSlot SpriteZone::getFromLeft(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            throw std::out_of_range("/!\\ SpriteZone coordinates out of range");

        return zone_[y][width_ - 1 - x];
    }

    void SpriteZone::flip_horizontal() {
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_ / 2; x++) {
                SpriteSlot temp = zone_[y][x];
                zone_[y][x] = zone_[y][width_ - 1 - x];
                zone_[y][width_ - 1 - x] = temp;
            }
        }
    }

    void SpriteZone::flip_vertical() {
        for (int y = 0; y < height_ / 2; y++) {
            for (int x = 0; x < width_; x++) {
                SpriteSlot temp = zone_[y][x];
                zone_[y][x] = zone_[height_ - 1 - y][x];
                zone_[height_ - 1 - y][x] = temp;
            }
        }
    }
}
