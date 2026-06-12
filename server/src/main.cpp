#include <chrono>
#include <iostream>
#include <thread>

#include "core/Args.hpp"
#include "core/Scheduler.hpp"
#include "core/World.hpp"

static void smokeScheduler()
{
    std::cout << "=== Scheduler smoke test ===\n";
    Scheduler sched;

    sched.schedule(std::chrono::milliseconds(1000), [] { std::cout << "fired at 1000ms\n"; });
    sched.schedule(std::chrono::milliseconds(3000), [] { std::cout << "fired at 3000ms\n"; });
    sched.schedule(std::chrono::milliseconds(2000), [] { std::cout << "fired at 2000ms\n"; });

    while (true) {
        int wait = sched.msUntilNext();
        if (wait == -1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(wait == 0 ? 1 : wait));
        sched.tick();
    }
    std::cout << "\n";
}

static void smokeWorld(const ServerConfig& config)
{
    std::cout << "=== World smoke test ===\n";
    World world(config.width, config.height, config.teamNames, config.clientsNb);
    world.spawnResources();

    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            auto& t = world.at(x, y);
            if (!t.resources.isEmpty())
                std::cout << "(" << x << "," << y << ") " << t.resources << "\n";
        }
    }

    int id = world.addPlayer(0, config.teamNames[0], config.width / 2, config.height / 2,
                             Orientation::N);
    std::cout << "Player added: id=" << id << "\n";
}

int main(int argc, char** argv)
{
    Args args(argc, argv);

    if (!args.isValid() || args.isHelpRequested()) return args.exitCode();

    ServerConfig config = args.getConfig();

    smokeScheduler();
    smokeWorld(config);

    return args.exitCode();
}
