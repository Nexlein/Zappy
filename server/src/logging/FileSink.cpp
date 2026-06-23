#include "logging/FileSink.hpp"

FileSink::FileSink(const std::string& path, LogLevel minLevel)
    : _out(path, std::ios::out | std::ios::trunc), _minLevel(minLevel)
{
}

void FileSink::write(LogLevel level, const std::string& line)
{
    if (level < _minLevel || !_out.is_open()) return;
    _out << line << "\n";
    _out.flush();
}
