#include "logging/FileSink.hpp"

FileSink::FileSink(const std::string& path) : _out(path, std::ios::out | std::ios::trunc) {}

void FileSink::write(const std::string& line)
{
    if (!_out.is_open()) return;
    _out << line << "\n";
    _out.flush();
}
