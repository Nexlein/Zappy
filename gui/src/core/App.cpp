#include "App.hpp"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

extern volatile sig_atomic_t g_interrupted;

#include "network/ProtocolParser.hpp"
#include "network/TcpSocket.hpp"
#include "renderer/HeadlessRenderer.hpp"
#include "renderer/IRenderer.hpp"
#include "renderer/RaylibRenderer.hpp"
#include "renderer/raylib_helpers/I18n.hpp"

static constexpr int MAX_RETRIES = 5;

App::App(int argc, char** argv) : args(argc, argv) {}

bool App::shouldRun() const { return args.isValid(); }

int App::exitCode() const { return args.exitCode(); }

void App::run()
{
    AppConfig config = args.getConfig();
    I18n::setLanguage(config.language);

    auto socket = std::make_unique<TcpSocket>();
    if (!_connectWithRetry(*socket, config.machine, config.port)) return;

    EventQueue eventQueue;
    IRenderer* renderer;

    if (config.headless) {
        std::cout << "[INFO] Running in headless mode\n";
        renderer = new HeadlessRenderer(std::cout);
    } else {
        renderer = new RaylibRenderer();
    }

    renderer->setDevMode(config.dev, config.port, config.machine);
    renderer->init();

    _rendererActive = true;
    while (!renderer->shouldClose() && !g_interrupted) {
        try {
            while (!renderer->shouldClose() && !g_interrupted) {
                socket->send("mct\n");
                _trySendStu(*socket);
                pollAndEnqueue(*socket, eventQueue);

                while (auto event = eventQueue.pop()) state.applyEvent(*event);

                renderer->setState(state);
                renderer->handleInput();
                renderer->render();
            }
        } catch (const TcpException& e) {
            std::cerr << "[Network] " << e.what() << "\n";
            renderer->shutdown();
            _rendererActive = false;
            socket = std::make_unique<TcpSocket>();
            if (!_connectWithRetry(*socket, config.machine, config.port)) break;
            state = GameState{};
            eventQueue.clear();
            _resetStuState();
            renderer->init();
            _rendererActive = true;
        }
    }

    if (g_interrupted) std::cerr << "[GUI] Stopping\n";
    if (_rendererActive) renderer->shutdown();
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

void App::_trySendStu(TcpSocket& socket)
{
    if (_stuSilenced) return;

    auto now = std::chrono::steady_clock::now();
    if (now - _lastStuSent < std::chrono::seconds(1)) return;

    if (_lastStuSent.time_since_epoch().count() != 0) {
        if (state.receivedStuResponse)
            _stuMissedResponses = 0;
        else if (++_stuMissedResponses >= 3) {
            _stuSilenced = true;
            std::cerr << "[Network] stu: no response after 3 attempts, disabling uptime polling\n";
            return;
        }
        state.receivedStuResponse = false;
    }

    socket.send("stu\n");
    _lastStuSent = now;
}

void App::_resetStuState()
{
    _lastStuSent = {};
    _stuMissedResponses = 0;
    _stuSilenced = false;
}

bool App::_connectWithRetry(TcpSocket& socket, const std::string& host, int port)
{
    int delay = 2;
    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        try {
            socket.connect(host, port);
            socket.send("GRAPHIC\n");
            std::cerr << "[Network] Connected to " << host << ":" << port << "\n";
            return true;
        } catch (const TcpException& e) {
            std::cerr << "[Network] " << e.what() << " (attempt " << attempt << "/" << MAX_RETRIES
                      << ")\n";
            if (attempt == MAX_RETRIES) break;
            for (int s = delay; s > 0 && !g_interrupted; s--) {
                std::cerr << "[Network] Retrying in " << s << "s...   \r";
                std::cerr.flush();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (g_interrupted) return false;
            delay *= 2;
        }
    }
    std::cerr << "\n[Network] Could not connect after " << MAX_RETRIES << " attempts.\n";
    return false;
}