#include "logging/Logger.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>

static const char* _levelName(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "?";
}

static std::string _timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
    localtime_r(&t, &tm);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec,
                  static_cast<int>(ms.count()));
    return buf;
}

Logger::Logger(LogLevel threshold) : _threshold(threshold) {}

void Logger::setSink(std::unique_ptr<ILogSink> sink) { _sink = std::move(sink); }

void Logger::_log(LogLevel level, std::string_view component, std::string_view msg)
{
    if (!_sink || level < _threshold) return;

    std::string line = "[" + _timestamp() + "] [" + _levelName(level) + "] [" +
                       std::string(component) + "] " + std::string(msg);
    _sink->write(line);
}

void Logger::debug(std::string_view component, std::string_view msg)
{
    _log(LogLevel::Debug, component, msg);
}

void Logger::info(std::string_view component, std::string_view msg)
{
    _log(LogLevel::Info, component, msg);
}

void Logger::warn(std::string_view component, std::string_view msg)
{
    _log(LogLevel::Warn, component, msg);
}

void Logger::error(std::string_view component, std::string_view msg)
{
    _log(LogLevel::Error, component, msg);
}
