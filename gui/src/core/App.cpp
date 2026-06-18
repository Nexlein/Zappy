#include "App.hpp"

#include <iostream>

#include "network/ProtocolParser.hpp"
#include "network/TcpSocket.hpp"
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
    try {
        while (!renderer->shouldClose()) {
            socket.send("mct\n");
            pollAndEnqueue(socket, eventQueue);

            while (auto event = eventQueue.pop()) {
                state.applyEvent(*event);
            }

            renderer->setState(state);
            renderer->handleInput();
            renderer->render();
        }
    } catch (const TcpException& e) {
        std::cerr << "[Network] " << e.what() << "\n";
    }

    renderer->shutdown();
    delete renderer;
}

void App::pollAndEnqueue(TcpSocket& socket, EventQueue& queue)
{
    while (socket.poll(0)) {
        std::optional<std::string> line = socket.recvLine();
        if (!line) break;
        std::optional<Event> event = ProtocolParser::parse(*line);
        if (event) queue.push(*event);
    }
}