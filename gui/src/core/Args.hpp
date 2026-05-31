#pragma once

#include <string>

struct AppConfig {
    int port;
    std::string machine;
    bool headless;
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
     * @brief Gets the configuration for the application.
     * @return The configuration.
     */
    AppConfig getConfig() const;

private:
    enum class ParseResult {
        Success,
        HelpRequested,
        Error
    };

    int SUCCESS = 0;
    int ERROR = 84;

    AppConfig config = {-1, "localhost", false};
    ParseResult result = ParseResult::Error;

    void parseArgs(int argc, char** argv);
    void setError(const std::string& message, const std::string& progName);
    static void printUsage(const std::string& progName);
};
