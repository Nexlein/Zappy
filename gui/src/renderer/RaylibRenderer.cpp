#include "RaylibRenderer.hpp"
#include <iostream>

void RaylibRenderer::init()
{
    std::cout << "[INFO] init()" << std::endl;
}

void RaylibRenderer::render(const GameState& state)
{
    std::cout << "[INFO] render()" << std::endl;
}

bool RaylibRenderer::shouldClose()
{
    std::cout << "[INFO] shouldClose()" << std::endl;
    return false;
}

void RaylibRenderer::shutdown()
{
    std::cout << "[INFO] shutdown()" << std::endl;
}
