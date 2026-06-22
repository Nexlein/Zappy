#include "logging/ConsoleSink.hpp"

#include <iostream>

ConsoleSink::ConsoleSink(LogLevel minLevel) : _minLevel(minLevel) {}

void ConsoleSink::write(LogLevel level, const std::string& line)
{
    if (level < _minLevel) return;
    std::cout << line << std::endl;
}
