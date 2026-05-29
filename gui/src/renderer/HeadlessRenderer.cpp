#include "HeadlessRenderer.hpp"

HeadlessRenderer::HeadlessRenderer(std::ostream& out)
    : _out(out)
{
}

void HeadlessRenderer::init()
{
}

void HeadlessRenderer::render(const WorldState& state)
{
    _out << "World: " << state.width << "x" << state.height
         << " | Players: " << state.players.size()
         << " | Eggs: " << state.eggs.size()
         << " | Teams: " << state.teams.size()
         << "\n";
}

bool HeadlessRenderer::shouldClose()
{
    return true;
}

void HeadlessRenderer::shutdown()
{
}
