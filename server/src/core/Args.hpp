#pragma once

#include <ostream>
#include <string>
#include <vector>

/**
 * @brief Represents the configuration for the server application, described by:
 * - port: the port number (-p)
 * - width: width of the world (-x)
 * - height: height of the world (-y)
 * - nameX: name of the teams (-n followed by team names)
 * - clientsNb: number of clients per team (-c)
 * - freq: frequency of the game (-f)
 */
struct ServerConfig {
    int port;
    int width;
    int height;
    std::vector<std::string> teamNames;
    int clientsNb;
    int freq;

    friend std::ostream& operator<<(std::ostream& os, const ServerConfig& c)
    {
        os << "Server configuration:\n"
           << "  Port    : " << c.port << "\n"
           << "  Map     : " << c.width << "x" << c.height << "\n"
           << "  Clients : " << c.clientsNb << " per team\n"
           << "  Freq    : " << c.freq << " Hz\n"
           << "  Teams   : ";
        for (size_t i = 0; i < c.teamNames.size(); ++i) {
            if (i) os << ", ";
            os << c.teamNames[i];
        }
        return os;
    }
};

class Args {
    public:
    Args(int argc, char** argv);

    /**
     * @brief Checks if the parsed arguments are valid and if the application should run.
     * @return true if the arguments are valid and the application should run, false otherwise.
     */
    bool isValid() const;

    /**
     * @brief Checks if the help flag was provided in the arguments.
     * @return true if the help flag was provided, false otherwise.
     */
    bool isHelpRequested() const;

    /**
     * @brief Gets the exit code for the application.
     * @return The exit code.
     */
    int exitCode() const;

    /**
     * @brief Gets the configuration for the server application.
     * @return The configuration.
     */
    ServerConfig getConfig() const;

    private:
    enum class ParseResult { Success, HelpRequested, Error };

    static constexpr int SUCCESS = 0;
    static constexpr int ERROR = 84;

    ServerConfig config = {-1, -1, -1, {}, 10, 100};
    ParseResult result = ParseResult::Error;
};
