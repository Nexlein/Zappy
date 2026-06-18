#pragma once

#include <string>

/**
 * @brief Output destination for a fully-formatted log line (Strategy pattern).
 *
 * The Logger does formatting and level filtering; a sink only writes the final
 * string somewhere (terminal, file, ...). Swap sinks without touching Logger.
 */
class ILogSink {
    public:
    virtual ~ILogSink() = default;

    /// Write one already-formatted line (newline appended by the sink).
    virtual void write(const std::string& line) = 0;
};
