#include "App.hpp"
#include "network/ProtocolParser.hpp"
#include "renderer/IRenderer.hpp"
#include "renderer/HeadlessRenderer.hpp"
#include "renderer/RaylibRenderer.hpp"
#include <iostream>

#include "network/ProtocolParser.hpp"
#include "renderer/HeadlessRenderer.hpp"
#include "renderer/IRenderer.hpp"
#include "renderer/RaylibRenderer.hpp"

App::App(int argc, char** argv) : args(argc, argv) {}

bool App::shouldRun() const { return args.isValid(); }

int App::exitCode() const { return args.exitCode(); }

void App::run()
{
    AppConfig config = args.getConfig();

    TcpSocket socket;
    socket.connect(config.machine, config.port);
    socket.send("GRAPHIC\n");

    EventQueue eventQueue;
    IRenderer* renderer;

    if (config.headless) {
        std::cout << "[INFO] Running in headless mode\n";
        renderer = new HeadlessRenderer(std::cout);
    } else {
        renderer = new RaylibRenderer();
    }

    renderer->init();
    while (!renderer->shouldClose()) {
        pollAndEnqueue(socket, eventQueue);

        while (auto event = eventQueue.pop()) {
            state.applyEvent(*event);
        }

        renderer->handleInput();
        renderer->render(state);
    }

    renderer->shutdown();
    delete renderer;
}

void App::pollAndEnqueue(TcpSocket& socket, EventQueue& queue)
{
    if (!socket.poll(0)) return;
    std::optional<std::string> line = socket.recvLine();
    if (!line) return;
    std::optional<Event> event = ProtocolParser::parse(*line);
    if (!event) return;
    queue.push(*event);
}