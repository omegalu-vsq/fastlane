#include "video.hh"

namespace fast {
    class Engine {
        Video video_;
        float sprite_clipping_;
        float drawing_distance;

        public:

        Engine(int width, int height, float clipping, float distance);

        // Calculation methods
        void basic();
        void scanline();
        void projection2();
        void projection22();
        void power2(); // 0/20
    };
}