#include "HeadlessRenderer.hpp"

HeadlessRenderer::HeadlessRenderer(std::ostream& out)
    : _out(out)
{
}

void HeadlessRenderer::init()
{
}

void HeadlessRenderer::render(const GameState& state)
{
    std::string statusStr;
    switch (state.status) {
        case ConnectionStatus::Disconnected: statusStr = "Disconnected"; break;
        case ConnectionStatus::Connecting: statusStr = "Connecting"; break;
        case ConnectionStatus::Connected: statusStr = "Connected"; break;
        case ConnectionStatus::Error: statusStr = "Error"; break;
    }

    std::string winnerTeamStr = state.winnerTeam.empty() ? "None" : state.winnerTeam;

    // Lambda to print Orientation
    auto orientationToStr = [](Orientation o) {
        switch (o) {
            case Orientation::N: return "N";
            case Orientation::E: return "E";
            case Orientation::S: return "S";
            case Orientation::W: return "W";
        }
        return "?";
    };

    _out << "Connection status: " << statusStr << "\n";
    _out << "Time unit: " << state.timeUnit << "\n";

    _out << "Teams (" << state.world.teams.size() << "): ";
    for (size_t i = 0; i < state.world.teams.size(); ++i) {
        _out << state.world.teams[i];
        if (i < state.world.teams.size() - 1) {
            _out << ", ";
        }
    }
    _out << "\n";

    _out << "Players (" << state.world.players.size() << "):\n";
    for (const auto& [id, player] : state.world.players) {
        _out << "  Player " << id << ": (" << player.x << ", " << player.y << "), facing "
            << orientationToStr(player.orientation) << ", team " << player.team
            << ", TTL: " << player.timeToLive << "\n";
    }

    _out << "Eggs (" << state.world.eggs.size() << "):\n";
    for (const auto& [id, egg] : state.world.eggs) {
        _out << "  Egg " << id << ": (" << egg.x << ", " << egg.y << "), team " << egg.team << "\n";
    }

    _out << "Winner team: " << winnerTeamStr << "\n";
}

bool HeadlessRenderer::shouldClose()
{
    return false; // Does not matter
}

void HeadlessRenderer::shutdown()
{
}
