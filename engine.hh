#include "video.hh"

namespace fast {
    class Engine {
        Video video_;

        public:

        Engine(int width, int height);

        // Calculation methods
        void basic();
        void projection();
        void projection2();
        void power2(); // 0/20
    };
}