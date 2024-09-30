#include "Logger.h"
#include <stdarg.h>

LogLevel Logger::currentLevel = LogLevel::INFO;
const char* Logger::levelStrings[] = {"NONE", "ERROR", "WARNING", "INFO", "DEBUG"};

void Logger::begin(unsigned long baud, LogLevel level) {
    Serial.begin(baud);
    currentLevel = level;
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::log(LogLevel level, const char* tag, const char* message) {
    if (level <= currentLevel) {
        Serial.printf("[%s] %s: %s\n", levelStrings[static_cast<int>(level)], tag, message);
    }
}

void Logger::logf(LogLevel level, const char* tag, const char* format, ...) {
    if (level <= currentLevel) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        log(level, tag, buffer);
    }
}