#include "Args.hpp"

#include <algorithm>
#include <iostream>

#include "clap/App.hpp"

Args::Args(int argc, char** argv)
{
    clap::App app(argv[0], "Zappy server");

    auto& port = app.option<int>("-p", "Listening port").required();
    auto& width = app.option<int>("-x", "Map width").required();
    auto& height = app.option<int>("-y", "Map height").required();
    auto& teams = app.multi_option<std::string>("-n", "Team names").required();
    auto& clientsNb = app.option<int>("-c", "Clients per team (default: 10)").default_value(10);
    auto& freq = app.option<int>("-f", "Time unit frequency (default: 100)").default_value(100);

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        result = ParseResult::HelpRequested;
        return;
    } catch (const clap::ClapException& e) {
        std::cerr << e.what() << "\n";
        result = ParseResult::Error;
        return;
    }

    auto names = teams.get();
    if (std::find(names.begin(), names.end(), "GRAPHIC") != names.end()) {
        std::cerr << "Team name 'GRAPHIC' is reserved and cannot be used.\n";
        result = ParseResult::Error;
        return;
    }

    int p = port.get(), w = width.get(), h = height.get();
    int c = clientsNb.get(), f = freq.get();

    if (p < 1 || p > 65535) {
        std::cerr << "Port must be between 1 and 65535.\n";
        result = ParseResult::Error;
        return;
    }
    if (w < 1 || h < 1) {
        std::cerr << "Map dimensions must be positive.\n";
        result = ParseResult::Error;
        return;
    }
    if (c < 1) {
        std::cerr << "Clients per team must be positive.\n";
        result = ParseResult::Error;
        return;
    }
    if (f < 1) {
        std::cerr << "Frequency must be positive.\n";
        result = ParseResult::Error;
        return;
    }

    config.port = p;
    config.width = w;
    config.height = h;
    config.teamNames = names;
    config.clientsNb = c;
    config.freq = f;
    result = ParseResult::Success;
}

bool Args::isValid() const { return result == ParseResult::Success; }
bool Args::isHelpRequested() const { return result == ParseResult::HelpRequested; }
ServerConfig Args::getConfig() const { return config; }

int Args::exitCode() const { return result == ParseResult::Error ? ERROR : SUCCESS; }
