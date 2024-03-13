#include <cstring>
#include <format>

#include "server.h"
#include "log.h"

void die(const std::string& msg) {
    log(LogLevel::ERROR, std::strerror(errno) + std::format(": {}", msg));
    std::exit(EXIT_FAILURE);
}
