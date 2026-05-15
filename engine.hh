#include "video.hh"

namespace fast {
    class Engine {
        Video video_;
        float sprite_clipping_;
        float drawing_distance;

        public:

        Engine(int width, int height, float clipping, float distance);

        // le plus moderne des moteurs, projection22() est basé sur projection2() en rajoutant principalement les objets Road et Camera
        void projection22();

        // Legacy renderers live in engineLegacy.cpp and are disabled by default.
        // Compile them with `make legacy` or `make LEGACY=1`.
#ifdef LEGACY
        // basic() est un moteur de type scanline avec très peu de fonctionnalités
        void basic();

        // scanline() est un moteur de type scanline, c'est à dire qu'il parcourt les lignes de l'écran puis trace le monde.
        // il est très performant mais n'est capable de tracer que des lignes droites, il n'y a donc pas d'altitude ni de courbes.
        void scanline();

        // projection2() fonctionne, à l'inverse des 2 moteurs précédents, en projetant le monde sur l'écran.
        // c'est à dire qu'au lieu de parcourir l'écran et de tracer le monde, on parcourt le monde, puis on le trace sur l'écran en interpollant.
        // ce type de rendu permet de créer des collines et des virages sans tricher, grâce à la formule de projection
        void projection2();

        // power2() n'est qu'une ébauche, il permet simplement de tracer des bandes de couleur pour simuler une profondeur
        void power2(); // 0/20
#endif
    };
}
