#pragma once

#include <fstream>
#include <string>

#include "logging/ILogSink.hpp"

/// Appends log lines to a file, flushing each write so the log survives a crash.
class FileSink : public ILogSink {
    public:
    explicit FileSink(const std::string& path);
    void write(const std::string& line) override;

    private:
    std::ofstream _out;
};
