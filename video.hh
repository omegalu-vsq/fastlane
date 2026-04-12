#pragma once

#include <cstdlib>
#include <cstdint>
#include <string>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>


namespace fast {

    class Color {
        public:
            uint8_t r;
            uint8_t g;
            uint8_t b;

        Color() = default;
        Color(uint8_t red, uint8_t green, uint8_t blue);

        friend bool operator==(const Color& lhs, const Color& rhs); 
        friend bool operator!=(const Color& lhs, const Color& rhs); 
    };
    
    namespace Colors {
        inline const Color RED{255, 0, 0};
        inline const Color GREEN{0, 255, 0};
        inline const Color BLUE{0, 0, 255};
        inline const Color WHITE{255, 255, 255};
        inline const Color BLACK{0, 0, 0};
        inline const Color JADE{75, 192, 173};
        inline const Color JADE2{72, 143, 131};
        inline const Color DARK_BLUE{0, 79, 163};
        inline const Color GRAY{69, 69, 61};
    }

    class Image {
        public:
            int width_;
            int height_;

            Color* pixels;
            std::size_t length;

            Image() = default;
            Image(int width, int height);

            ~Image();

            void save(std::string name);
    };

    class Video {
        public:
            int width_;
            int height_;
            Image buffer_;

            SDL_Window* window_;
            SDL_Renderer* renderer_;
            SDL_Texture* texture_;

            Video(int width, int height);
            ~Video();

    };
}
