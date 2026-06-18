#pragma once

#include "logging/ILogSink.hpp"

/// Writes log lines to the terminal (stdout).
class ConsoleSink : public ILogSink {
    public:
    void write(const std::string& line) override;
};
