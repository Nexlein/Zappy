#include "Args.hpp"

#include <algorithm>
#include <iostream>
#include <random>

#include "clap/App.hpp"

namespace {

    void failWith(const std::string& msg, const char* prog)
    {
        std::cerr << msg << "\nTry '" << prog << " --help' for more information.\n";
    }

}  // namespace

Args::Args(int argc, char** argv)
{
    clap::App app(argv[0], "Zappy server");

    auto& port = app.option<int>("-p", "Listening port").required();
    auto& width = app.option<int>("-x", "Map width").required();
    auto& height = app.option<int>("-y", "Map height").required();
    auto& teams = app.multi_option<std::string>("-n", "Team names").required();
    auto& clientsNb = app.option<int>("-c", "Clients per team (default: 10)").default_value(10);
    auto& freq = app.option<int>("-f", "Time unit frequency (default: 100)").default_value(100);
    auto& seed = app.option<int>("-s,--seed", "RNG seed for reproducible games (default: random)")
                     .default_value(-1);

    if (argc == 1) {
        app.print_help();
        result = ParseResult::HelpRequested;
        return;
    }

    try {
        app.parse(argc, argv);
    } catch (const clap::HelpRequested&) {
        result = ParseResult::HelpRequested;
        return;
    } catch (const clap::ClapException& e) {
        failWith(e.what(), argv[0]);
        result = ParseResult::Error;
        return;
    }

    auto names = teams.get();
    if (std::find(names.begin(), names.end(), "GRAPHIC") != names.end()) {
        failWith("Team name 'GRAPHIC' is reserved and cannot be used.", argv[0]);
        result = ParseResult::Error;
        return;
    }

    int p = port.get(), w = width.get(), h = height.get();
    int c = clientsNb.get(), f = freq.get(), s = seed.get();

    if (p < 1 || p > 65535) {
        failWith("Port must be between 1 and 65535.", argv[0]);
        result = ParseResult::Error;
        return;
    }
    if (w < 1 || h < 1) {
        failWith("Map dimensions must be positive.", argv[0]);
        result = ParseResult::Error;
        return;
    }
    if (c < 1) {
        failWith("Clients per team must be positive.", argv[0]);
        result = ParseResult::Error;
        return;
    }
    if (f < 1) {
        failWith("Frequency must be positive.", argv[0]);
        result = ParseResult::Error;
        return;
    }

    config.port = p;
    config.width = w;
    config.height = h;
    config.teamNames = names;
    config.clientsNb = c;
    config.freq = f;
    config.seed = (s < 0) ? std::random_device{}() : static_cast<unsigned int>(s);
    result = ParseResult::Success;
}

bool Args::isValid() const { return result == ParseResult::Success; }
bool Args::isHelpRequested() const { return result == ParseResult::HelpRequested; }
ServerConfig Args::getConfig() const { return config; }

int Args::exitCode() const { return result == ParseResult::Error ? ERROR : SUCCESS; }
