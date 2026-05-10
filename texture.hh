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
    
    class SkyTexture : public Texture {
        public:
            float shiftStrength_;
            
            static SkyTexture load(const std::string& filename, float strength);
    };

    class Sprite {
        public:
            Texture texture_;
            float scale_;

            Sprite() = default;
            Sprite(const std::string& texture, float scale);
    };
    
    struct SpriteSlot {
        bool active;
        Sprite sprite_;
    };

    class SpriteZone {
        public:
            int width_;
            int height_;
            float roadOffset_;
            float widthScale_;
            std::vector<std::vector<SpriteSlot>> zone_;

            SpriteZone() = default;
            SpriteZone(int width, int height);

            static SpriteZone load(const std::string& filename);

            SpriteSlot get(int x, int y) const;
            SpriteSlot getFromLeft(int x, int y) const;

            void setRoadOffset(float offset) { roadOffset_ = offset; }
            void setWidthScale(float scale) { widthScale_ = scale; }

            void flip_horizontal();
            void flip_vertical();
    };

}
