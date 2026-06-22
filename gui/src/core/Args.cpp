#include "Args.hpp"

#include <iostream>

Args::Args(int argc, char** argv) { parseArgs(argc, argv); }

bool Args::isValid() const { return result == ParseResult::Success; }

bool Args::isHelpRequested() const { return result == ParseResult::HelpRequested; }

int Args::exitCode() const
{
    if (result == ParseResult::Success || result == ParseResult::HelpRequested) {
        return SUCCESS;
    }
    return ERROR;
}

AppConfig Args::getConfig() const { return config; }

void Args::parseArgs(int argc, char** argv)
{
    if (argc < 2) {
        setError("No arguments provided", argv[0]);
        return;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printUsage(argv[0]);
            result = ParseResult::HelpRequested;
            return;
        } else if (arg == "--headless") {
            config.headless = true;
        } else if (arg == "--dev") {
            if (i + 1 >= argc) {
                setError("--dev requires a value (true or false)", argv[0]);
                return;
            }
            std::string val = argv[++i];
            if (val == "true")
                config.dev = true;
            else if (val == "false")
                config.dev = false;
            else {
                setError("--dev value must be true or false", argv[0]);
                return;
            }
        } else if (arg == "--language") {
            if (i + 1 >= argc) {
                setError("--language requires a value (english or french)", argv[0]);
                return;
            }
            std::string lang = argv[++i];
            if (lang == "french")
                config.language = I18n::Language::FR;
            else if (lang == "english")
                config.language = I18n::Language::EN;
            else {
                setError("Unknown language: " + lang + " (use english or french)", argv[0]);
                return;
            }
        } else if (arg == "-p") {
            if (i + 1 >= argc) {
                setError("-p requires a port number", argv[0]);
                return;
            }
            try {
                config.port = std::stoi(argv[i + 1]);
                if (config.port <= 0 || config.port > 65535) {
                    setError("Port must be between 1 and 65535", argv[0]);
                    return;
                }
                ++i;
            } catch (const std::exception&) {
                setError("Invalid port number: " + std::string(argv[i + 1]), argv[0]);
                return;
            }
        } else if (arg == "-h") {
            if (i + 1 >= argc) {
                setError("-h requires a machine name", argv[0]);
                return;
            }
            config.machine = argv[i + 1];
            ++i;
        } else {
            setError("Unknown argument: " + arg, argv[0]);
            return;
        }
    }

    if (config.port == -1) {
        setError("-p port is required", argv[0]);
        return;
    }

    result = ParseResult::Success;
}

void Args::setError(const std::string& message, const std::string& progName)
{
    std::cerr << "Error: " << message << "\n";
    printUsage(progName);
    result = ParseResult::Error;
}

void Args::printUsage(const std::string& progName)
{
    std::cerr << "Usage: " << progName
              << " -p port [-h machine] [--headless] [--dev true|false] [--language english|french]\n";
    std::cerr << "  -p port                    Port number to connect to (required)\n";
    std::cerr << "  -h machine                 Machine name or IP address (default: localhost)\n";
    std::cerr << "  --headless                 Run without graphics (text output only)\n";
    std::cerr << "  --dev true|false           Show dev HUD (FPS, time unit, connection info)\n";
    std::cerr << "  --language english|french  UI language (default: english)\n";
    std::cerr << "  --help                     Show this help message\n";
}
