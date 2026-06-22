#pragma once

#include <memory>
#include <vector>

#include "logging/ILogSink.hpp"

/// Fans one write out to many sinks at once (Composite pattern).
class CompositeSink : public ILogSink {
    public:
    void add(std::unique_ptr<ILogSink> sink);
    void write(LogLevel level, const std::string& line) override;

    private:
    std::vector<std::unique_ptr<ILogSink>> _sinks;
};
