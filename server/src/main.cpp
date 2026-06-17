#include "core/Args.hpp"
#include "core/Server.hpp"

int main(int argc, char** argv)
{
    Args args(argc, argv);
    if (!args.isValid() || args.isHelpRequested()) return args.exitCode();

    ServerConfig config = args.getConfig();
    Server server(config);

    return server.run();
}
