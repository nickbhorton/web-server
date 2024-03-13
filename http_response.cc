#include <format>
#include <sys/socket.h>

#include "http.h"
#include "log.h"
#include "server.h"


std::string generate_http_response(
    const std::map<std::string, std::string> headers,
    unsigned int response_status_code, 
    const std::string& response_string,
    std::optional<std::string> body,
    const std::string& version
) 
{
    std::stringstream response_stream;
    response_stream << std::format("HTTP/{} {} {}\r\n", version, response_status_code, response_string);
    for (const auto& header : headers) {
        response_stream << std::get<0>(header);
        response_stream << ": ";
        response_stream << std::get<1>(header);
        response_stream << "\r\n";
    }
    response_stream << "\r\n";
    if (body.has_value()) {
        response_stream << body.value();
    }
    return response_stream.str();
}

void send_http_response(int fd, const std::string& response) {
    int bytes_sent = send(fd, response.data(), response.length(), 0);
    if (bytes_sent < 0) {
        die("send");
    }
    log(LogLevel::DETAIL, "send_http_response write() returned: " + std::to_string(bytes_sent));
}
