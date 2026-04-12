#pragma once

#include "video.hh"
#include <string>
#include <vector>

namespace fast {
    
    class Texture {
        public:
            int width_;
            int height_;
            std::vector<std::vector<Color>> pixels_;

            Texture() = default;
            Texture(int width, int height);
            
            static Texture load(const std::string& filename);
            
            Color get(int x, int y) const;

            void flip_horizontal();
            void flip_vertical();
    };

}
