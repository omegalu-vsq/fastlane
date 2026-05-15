#include "engine.hh"

#include <iostream>

int main() {
    fast::Engine engine{480, 360, 10, 1000};

    engine.projection22();

    return 0;
}