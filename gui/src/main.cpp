#include "WorldState.hpp"
#include "renderer/HeadlessRenderer.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    WorldState state;
    state.width = 10;
    state.height = 10;

    HeadlessRenderer renderer(std::cout);
    renderer.init();
    renderer.render(state);
    renderer.shutdown();

    return 0;
}
