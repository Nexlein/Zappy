#include "logging/FileSink.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <sstream>

FileSink::FileSink(const std::string& path, LogLevel minLevel)
    : _out(path, std::ios::out | std::ios::trunc), _minLevel(minLevel)
{
}

std::unique_ptr<FileSink> FileSink::forRun(const std::string& tag, LogLevel minLevel)
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_r(&now, &tm);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &tm);

    std::error_code ec;
    std::filesystem::create_directories("server/logs", ec);

    std::ostringstream path;
    path << "server/logs/" << tag << "_" << stamp << ".log";
    return std::make_unique<FileSink>(path.str(), minLevel);
}

void FileSink::write(LogLevel level, const std::string& line)
{
    if (level < _minLevel || !_out.is_open()) return;
    _out << line << "\n";
    _out.flush();
}
