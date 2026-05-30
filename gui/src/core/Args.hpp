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

    bool isValid() const;
    bool isHelpRequested() const;
    int exitCode() const;
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
