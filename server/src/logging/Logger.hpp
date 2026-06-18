#pragma once

#include <memory>
#include <string_view>

#include "logging/ILogSink.hpp"

enum class LogLevel { Debug = 0, Info, Warn, Error };

/**
 * @brief Logging facade: formats `[time] [LEVEL] [component] message`, filters
 * by level, and forwards to a single sink (typically a CompositeSink).
 *
 * Sink and threshold are injected (Dependency Injection). Messages below the
 * threshold are dropped before formatting.
 *
 * @see server issue #127 — Logger system.
 */
class Logger {
    public:
    explicit Logger(LogLevel threshold = LogLevel::Info);

    void setSink(std::unique_ptr<ILogSink> sink);
    void setThreshold(LogLevel threshold) { _threshold = threshold; }

    void debug(std::string_view component, std::string_view msg);
    void info(std::string_view component, std::string_view msg);
    void warn(std::string_view component, std::string_view msg);
    void error(std::string_view component, std::string_view msg);

    private:
    void _log(LogLevel level, std::string_view component, std::string_view msg);

    std::unique_ptr<ILogSink> _sink;
    LogLevel _threshold;
};
