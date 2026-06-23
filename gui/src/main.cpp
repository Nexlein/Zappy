#include <csignal>
#include <iostream>

#include "core/App.hpp"

volatile sig_atomic_t g_interrupted = 0;

static void onInterrupt(int) { g_interrupted = 1; }

int main(int argc, char** argv)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, onInterrupt);
    signal(SIGTERM, onInterrupt);

    App app(argc, argv);

    if (!app.shouldRun()) {
        return app.exitCode();
    }

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[Fatal] Unknown exception\n";
        return 1;
    }
    return 0;
}
