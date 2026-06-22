#pragma once

#include <string>

enum class LogLevel { Debug = 0, Info, Warn, Error };

/**
 * @brief Output destination for a fully-formatted log line (Strategy pattern).
 *
 * The Logger formats the line; each sink decides whether to write it based on
 * its own minimum level. This lets the file keep everything (Debug) while the
 * console stays readable (Info). Swap sinks without touching Logger.
 */
class ILogSink {
    public:
    virtual ~ILogSink() = default;

    /// Write one already-formatted line if @p level passes the sink's threshold.
    virtual void write(LogLevel level, const std::string& line) = 0;
};
