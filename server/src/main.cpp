#include "core/Args.hpp"
#include "core/Server.hpp"

int main(int argc, char** argv)
{
    Args args(argc, argv);
    if (!args.isValid() || args.isHelpRequested()) return args.exitCode();

    Server server(args.getConfig());
    server.run();

    return 0;
}
