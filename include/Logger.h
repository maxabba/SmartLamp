#pragma once
#include <Arduino.h>

enum class LogLevel {
    NONE,
    ERROR,
    WARNING,
    INFO,
    DEBUG
};

class Logger {
private:
    static LogLevel currentLevel;
    static const char* levelStrings[];

public:
    static void begin(unsigned long baud, LogLevel level = LogLevel::INFO);
    static void setLogLevel(LogLevel level);
    static void log(LogLevel level, const char* tag, const char* message);
    static void logf(LogLevel level, const char* tag, const char* format, ...);
};

#define LOG_ERROR(tag, ...) Logger::logf(LogLevel::ERROR, tag, __VA_ARGS__)
#define LOG_WARNING(tag, ...) Logger::logf(LogLevel::WARNING, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...) Logger::logf(LogLevel::INFO, tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...) Logger::logf(LogLevel::DEBUG, tag, __VA_ARGS__)