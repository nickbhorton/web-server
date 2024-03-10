#ifndef LOG_HEADER
#define LOG_HEADER

#include <iostream>

enum class LogLevel {
    DETAIL,
    INFO,
    WARNING,
    ERROR
};

void log(LogLevel level, const std::string& message, std::ostream& os = std::cout);

#endif
