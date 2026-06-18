#include "logging/CompositeSink.hpp"

void CompositeSink::add(std::unique_ptr<ILogSink> sink) { _sinks.push_back(std::move(sink)); }

void CompositeSink::write(const std::string& line)
{
    for (auto& sink : _sinks) sink->write(line);
}
