#include "HeadlessRenderer.hpp"

HeadlessRenderer::HeadlessRenderer(std::ostream& out) : _out(out) {}

void HeadlessRenderer::init() {}

void HeadlessRenderer::render(const GameState& state)
{
    if (!state.isDirty()) return;

    std::string winnerTeamStr = state.winnerTeam.empty() ? "None" : state.winnerTeam;

    _out << "\n=== Game State Update ===\n";
    _out << "Time unit: " << state.timeUnit << "\n";
    _out << "Winner team: " << winnerTeamStr << "\n";
    _out << "Map " << state.world.width << "x" << state.world.height << "\n";

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
        _out << player << "\n";
    }

    _out << "Eggs (" << state.world.eggs.size() << "):\n";
    for (const auto& [id, egg] : state.world.eggs) {
        _out << egg << "\n";
    }

    state.clearDirty();
}

void HeadlessRenderer::handleInput() {}

bool HeadlessRenderer::shouldClose()
{
    return false;  // Does not matter
}

void HeadlessRenderer::shutdown() {}
