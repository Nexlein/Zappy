#include <iostream>

#include "core/Args.hpp"
#include "core/Server.hpp"

int main(int argc, char** argv)
{
    Args args(argc, argv);
    if (!args.isValid() || args.isHelpRequested()) return args.exitCode();

    try {
        Server server(args.getConfig());
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
