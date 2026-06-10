#include "HeadlessRenderer.hpp"

HeadlessRenderer::HeadlessRenderer(std::ostream& out) : _out(out) {}

void HeadlessRenderer::init() {}

void HeadlessRenderer::render()
{
    if (!_state || !_state->isDirty()) return;

    std::string winnerTeamStr = _state->winnerTeam.empty() ? "None" : _state->winnerTeam;

    _out << "\n=== Game State Update ===\n";
    _out << "Time unit: " << _state->timeUnit << "\n";
    _out << "Winner team: " << winnerTeamStr << "\n";
    _out << "Map " << _state->world.width << "x" << _state->world.height << "\n";

    _out << "Teams (" << _state->world.teams.size() << "): ";
    for (size_t i = 0; i < _state->world.teams.size(); ++i) {
        _out << _state->world.teams[i];
        if (i < _state->world.teams.size() - 1) {
            _out << ", ";
        }
    }
    _out << "\n";

    _out << "Players (" << _state->world.players.size() << "):\n";
    for (const auto& [id, player] : _state->world.players) {
        _out << player << "\n";
    }

    _out << "Eggs (" << _state->world.eggs.size() << "):\n";
    for (const auto& [id, egg] : _state->world.eggs) {
        _out << egg << "\n";
    }

    _state->clearDirty();
}

void HeadlessRenderer::handleInput() {}

bool HeadlessRenderer::shouldClose()
{
    return false;  // Does not matter
}

void HeadlessRenderer::shutdown() {}
