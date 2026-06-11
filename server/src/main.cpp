#include <iostream>

#include "core/Args.hpp"
#include "core/World.hpp"

int main(int argc, char **argv)
{
    Args args(argc, argv);

    if (!args.isValid() || args.isHelpRequested()) {
        return args.exitCode();
    }

    ServerConfig config = args.getConfig();
    std::cout << config << std::endl;

    World world(config.width, config.height, config.teamNames, config.clientsNb);
    world.spawnResources();

    return args.exitCode();
}
