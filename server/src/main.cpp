#include "core/Args.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    Args args(argc, argv);

    if (!args.isValid() || args.isHelpRequested()) {
        return args.exitCode();
    }

    ServerConfig config = args.getConfig();
    std::cout << config << std::endl;
    return args.exitCode();
}
