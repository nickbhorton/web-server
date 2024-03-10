#include "log.h"

std::ostream& operator<<(std::ostream & os, LogLevel level) {
    switch (level) {
        case LogLevel::DETAIL:
            os << "DETAIL";
            break;
        case LogLevel::INFO:
            os << "\033[32mINFO\033[m";
            break;
        case LogLevel::WARNING:
            os << "\033[33mWARNING\033[m";
            break;
        case LogLevel::ERROR:
            os << "\033[31mERROR\033[m";
            break;
    }
    return os;
}

constexpr LogLevel PrintLogLevel = LogLevel::DETAIL;
void log(LogLevel level, const std::string& message, std::ostream& os) {
    if (level >= PrintLogLevel) {
        os << "[" << level << "]: ";
        os << message;
        os << "\n" << std::flush;
    }
}
