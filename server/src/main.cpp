#include <iostream>

#include "core/Args.hpp"
#include "core/World.hpp"

int main(int argc, char** argv)
{
    Args args(argc, argv);

    if (!args.isValid() || args.isHelpRequested()) {
        return args.exitCode();
    }

    ServerConfig config = args.getConfig();

    World world(config.width, config.height, config.teamNames, config.clientsNb);
    world.spawnResources();

    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            auto& t = world.at(x, y);
            if (!t.resources.isEmpty())
                std::cout << "(" << x << "," << y << ") " << t.resources << "\n";
        }
    }

    int id = world.addPlayer(0, "TeamA", 5, 5, Orientation::N);
    std::cout << "Player added: id=" << id << "\n";

    return args.exitCode();
}
