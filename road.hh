#pragma once

#include <string>
#include <vector>

namespace fast {
    class Road {
        public:
        //SpriteZone sprite_zone;

        // road segmentation in world unit z (depth)
        float strip_size = 512.0f;
        float segment_length;

        float width; // screen scaled
        float grass_width; // screen scaled
        float& zref; // reference for cross multiplications for all screen scaled values
        
        // deformation effects
        float curve_strength = 200.0f;
        float altitude_strength = 1.0f;
        float altitude_step = strip_size * 2.0f;
        float tension = 0.5f; // catmull rom parameter

        std::vector<float> altitudes; // based on world unit y
        std::vector<float> centers; // normalized screen space offset

        //Texture road_texture;
        //Texture grass_texture;


        Road() = delete;

        Road(float width, float& ref);

        float catmullRomSpline(std::vector<float>& v, float t, int i, float tens);
        float catmullRomSplineDerived(std::vector<float>& v, float t, int i, float tens);

        float trackXAt(float z);
        float altitude(float z);
        float altitudeDerived(float z);

        void parseTrack(const std::string& filename);

        void setRoadTexture(const std::string& filename);
        void setGrassTexture(const std::string& filename);
        void setSpriteZone(const std::string& filename);

    };
}
