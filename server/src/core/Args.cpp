#include "Args.hpp"
#include "clap/App.hpp"

#include <algorithm>
#include <iostream>

Args::Args(int argc, char** argv) {
    clap::App app(argv[0], "Zappy server");

    auto& port      = app.option<int>("-p", "Listening port").required();
    auto& width     = app.option<int>("-x", "Map width").required();
    auto& height    = app.option<int>("-y", "Map height").required();
    auto& teams     = app.multi_option<std::string>("-n", "Team names").required();
    auto& clientsNb = app.option<int>("-c", "Clients per team (default: 10)").default_value(10);
    auto& freq      = app.option<int>("-f", "Time unit frequency (default: 100)").default_value(100);

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
        result = ParseResult::Error;
        return;
    }

    config.port      = port.get();
    config.width     = width.get();
    config.height    = height.get();
    config.teamNames = names;
    config.clientsNb = clientsNb.get();
    config.freq      = freq.get();
    result = ParseResult::Success;
}

bool Args::isValid() const        { return result == ParseResult::Success; }
bool Args::isHelpRequested() const { return result == ParseResult::HelpRequested; }
ServerConfig Args::getConfig() const { return config; }

int Args::exitCode() const {
    return result == ParseResult::Success ? SUCCESS : ERROR;
}
