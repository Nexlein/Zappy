#include "core/App.hpp"

int main(int argc, char** argv)
{
    App app(argc, argv);

    if (!app.shouldRun()) {
        return app.exitCode();
    }

    app.run();
    return 0;
}
