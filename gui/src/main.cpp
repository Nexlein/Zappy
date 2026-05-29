#include "core/GameState.hpp"
#include "core/Event.hpp"
#include "renderer/HeadlessRenderer.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    GameState state;
    state.applyEvent(MapSize{10, 10});
    state.applyEvent(TeamName{"Red"});
    state.applyEvent(TeamName{"Blue"});
    state.applyEvent(PlayerNew{1, 2, 3, Orientation::N, 1, "Red"});
    state.applyEvent(PlayerNew{2, 5, 6, Orientation::E, 1, "Blue"});
    state.applyEvent(PlayerNew{3, 7, 8, Orientation::S, 1, "Red"});
    state.applyEvent(PlayerPosition{1, 6, 7, Orientation::W});
    state.applyEvent(PlayerLevel{1, 2});
    state.applyEvent(TimeUnit{42});
    state.applyEvent(EggNew{1, 1, 4, 4});
    state.applyEvent(GameEnd{"Red"});

    HeadlessRenderer renderer(std::cout);
    renderer.init();
    renderer.render(state);
    renderer.shutdown();

    return 0;
}
