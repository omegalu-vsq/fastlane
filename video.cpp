#include "video.hh"

#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

namespace fast {

    Color::Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue)
    {}

    bool operator==(const Color& lhs, const Color& rhs) {
        if (lhs.r != rhs.r) return false;
        if (lhs.g != rhs.g) return false;
        if (lhs.b != rhs.b) return false;
        return true;
    }
    
    bool operator!=(const Color& lhs, const Color& rhs) {
        if (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b) return false;
        return true;
    }

    Image::Image(int width, int height) {
        height_ = height;
        width_ = width;
        pixels = static_cast<Color*>(std::calloc(width * height, sizeof(Color)));
    }

    Image::~Image() {
        std::free(pixels);
    }

    void Image::save(std::string name) {
        std::ofstream file;
        file.open(name);
        file << "P3\n";
        file << width_ << "\n";
        file << height_ << "\n";
        file << "255\n";

        for (int i = 0; i < height_; i++) {
            for (int j = 0; j < width_; j++) {
                file << int(pixels[i * width_ + j].r) << ' ';
                file << int(pixels[i * width_ + j].g) << ' ';
                file << int(pixels[i * width_ + j].b) << ' ';
            }

            file << '\n';
        }

        file.close();
    }

    Video::Video(int width, int height) : width_(width), height_(height), buffer_(width, height) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "/!\\ SDL_Init() error: " << SDL_GetError() << std::endl;
            return;
        }

        window_ = SDL_CreateWindow("Fastlane Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
        if (!window_) {
            std::cerr << "/!\\ SDL_CreateWindow() error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) {
            std::cerr << "/!\\ SDL_CreateRenderer() error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window_);
            SDL_Quit();
            return;
        }
        
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (!texture_) {
            std::cerr << "/!\\ SDL_CreateTexture error: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(renderer_);
            SDL_DestroyWindow(window_);
            SDL_Quit();
            return;
        }
    }

    Video::~Video() {
        SDL_DestroyTexture(texture_);
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
    }
}
