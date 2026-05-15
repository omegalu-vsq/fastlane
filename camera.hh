#pragma once

namespace fast {
    class Camera {
        public:
        float distance; // distance between camera & screen
        float height_base; // from world unit y = 0 (altitude)
        float horizon_base; // idem, line of sight
        float lateral_pivot = 0.5f; // shift the projections to simulate a lateral pivot
        float lateral_pos = 0.5f; // screen scaled; from road

        float speed_base;

        bool yawed = true; // rotation activation for curve variations
        bool latfol = true; // lateral following activation for curve variations
        bool heightfol = true; // vertical following activation for altitude variations

        // for curve effects
        float turn_strength = 0.85f;
        float curve_lookahead; // arbitrary value, depends on the rendering you want

        // for altitude effects
        float tilt_strength = 3.5f;
        float tilt_smoothing = 8.0f;

        Camera() = delete;

        Camera(float dist, float height, float horizon, float speed) {
            this->distance = dist;
            this->height_base = height;
            this->horizon_base = horizon;
            this->speed_base = speed;
            this->curve_lookahead = dist;
        }
    };
}
