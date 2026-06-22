#pragma once

#include "logging/ILogSink.hpp"

/// Writes log lines to the terminal (stdout), filtered by a minimum level.
class ConsoleSink : public ILogSink {
    public:
    explicit ConsoleSink(LogLevel minLevel = LogLevel::Info);
    void write(LogLevel level, const std::string& line) override;

    private:
    LogLevel _minLevel;
};
