#pragma once

#include <fstream>
#include <memory>
#include <string>

#include "logging/ILogSink.hpp"

/// Appends log lines to a file (filtered by minimum level), flushing each write
/// so the log survives a crash.
class FileSink : public ILogSink {
    public:
    explicit FileSink(const std::string& path, LogLevel minLevel = LogLevel::Debug);

    /// Sink writing to @c logs/<tag>_<timestamp>.log (dir created best-effort),
    /// so concurrent and successive runs never clobber. @p tag e.g. "server_p4242".
    static std::unique_ptr<FileSink> forRun(
        const std::string& tag, LogLevel minLevel = LogLevel::Debug);

    void write(LogLevel level, const std::string& line) override;

    private:
    std::ofstream _out;
    LogLevel _minLevel;
};
