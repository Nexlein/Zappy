#include <iostream>

#include "core/App.hpp"

int main(int argc, char** argv)
{
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
