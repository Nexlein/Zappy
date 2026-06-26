#pragma once

#include <chrono>
#include <memory>

#include "../audio/IAudioManager.hpp"
#include "Args.hpp"
#include "EventQueue.hpp"
#include "GameState.hpp"
#include "network/TcpSocket.hpp"

/**
 * @brief The main application class that manages the game state, network communication, and
 * rendering. It parses command-line arguments, connects to the game server, receives events,
 * updates the game state, and renders the game using a renderer.
 */
class App {
    public:
    App(int argc, char** argv);

    /**
     * @brief Checks if the application should run based on the parsed arguments.
     * @return true if the application should run, false otherwise.
     */
    bool shouldRun() const;

    /**
     * @brief Gets the exit code to return if the application should not run.
     * @return The exit code to return.
     */
    int exitCode() const;

    /**
     * @brief Runs the application.
     * @note Should only be called if shouldRun() returns true.
     */
    void run();

    protected:  // <- Protected for testing purposes, acts as private in practice
    Args args;
    GameState state;
    bool _rendererActive = false;

    /**
     * @brief Polls the socket for new data and enqueues any received events.
     * @param socket The TCP socket to poll.
     * @param queue The event queue to enqueue events into.
     */
    void pollAndEnqueue(TcpSocket& socket, EventQueue& queue);

    /** @brief Attempts to connect with exponential backoff. Returns true on success. */
    bool _connectWithRetry(TcpSocket& socket, const std::string& host, int port);

    /** @brief Sends stu once per real second and silences it after 3 consecutive non-responses. */
    void _trySendStu(TcpSocket& socket);

    /** @brief Resets stu polling state, called on reconnect. */
    void _resetStuState();

    std::chrono::steady_clock::time_point _lastStuSent{};
    int _stuMissedResponses = 0;
    bool _stuSilenced = false;

    std::unique_ptr<IAudioManager> _audioManager;
};
