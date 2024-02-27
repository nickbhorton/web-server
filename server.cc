#include <iostream>
#include <cstring> // for std::strerror
#include <unistd.h>
#include <sstream>
#include <map>
#include <tuple>
#include <vector>
#include <format>

#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

std::ostream& operator<<(std::ostream & os, LogLevel level) {
    switch (level) {
        case LogLevel::INFO:
            os << "INFO";
            break;
        case LogLevel::WARNING:
            os << "WARNING";
            break;
        case LogLevel::ERROR:
            os << "ERROR";
            break;
    }
    return os;
}

void log(LogLevel level, const std::string& message, std::ostream& os = std::cout) {
    os << "[" << level << "]: ";
    os << message;
    os << "\n" << std::flush;
}

void die(const std::string& msg) {
    log(LogLevel::ERROR, std::strerror(errno));
    std::exit(EXIT_FAILURE);
}

std::ostream& operator<<(std::ostream & os, std::map<std::string, std::string> map) {
    if (map.size() <= 0) {
        return os;
    }
    std::map<std::string, std::string>::iterator it = map.begin();
    while (it != map.end()) {
        os << "Key: " << it->first
             << ", Value: " << it->second << "\n";
        ++it;
    }
    return os;
}

// https://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
std::string trim(const std::string& str,
                 const std::string& whitespace = " \t\n\r")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

enum class HttpMethod {
    GET
};

std::string to_string(HttpMethod m) {
    switch (m) {
        case HttpMethod::GET:
            return "GET";
            break;
    }
    return "";
}

std::ostream& operator<<(std::ostream & os, HttpMethod m) {
    os << to_string(m);
    return os;
}

std::tuple<HttpMethod, std::string, std::string> process_first_line(std::string first_line) {
    HttpMethod method = HttpMethod::GET;
    std::string path{};
    std::string version{};
    std::stringstream ss{first_line};
    std::string line{};
    std::vector<std::string> parameters;
    while (getline(ss, line, ' ')) {
        parameters.push_back(line);
    }
    if (parameters.size() < 3) {
        std::cerr << "not enough parameters for the http request\n"; 
    }
    else {
        if (trim(parameters[0]) == "GET") {
            method = HttpMethod::GET;
        }
        else {
            std::cerr << "unknow http method found: " << trim(parameters[0]) << "\n";
        }
        path = parameters[1];
        version = parameters[2];
    }
    return {method, path, version};
}


int main(int argc, char** argv) {
    if (argc != 3) {
        die("2 cmd args reqired | <address> <port>");
    }
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        die("socket");
    }
    log(LogLevel::INFO, "server socket: " + std::to_string(socket_fd));

    int option = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))  < 0) {
        die("setsockopt");
    }
    sockaddr_in server_address{};
    server_address.sin_family        = AF_INET;
    server_address.sin_port          = htons(std::stoi(argv[2]));
    server_address.sin_addr.s_addr   = inet_addr(argv[1]);
    if (bind(socket_fd, (sockaddr *)&server_address, sizeof(server_address))) {
        die("bind");
    }
    log(LogLevel::INFO, "server socket bound successfully");
    if (listen(socket_fd, 5)) {
        die("listen");
    }
    sockaddr_in connected_address{};
    socklen_t connected_address_size{};
    while (true) {
        log(LogLevel::INFO, "awaiting connection");
        int connection_fd = accept(socket_fd, (sockaddr *)&connected_address, &connected_address_size);
        if (connection_fd < 0) {
            die("accept");
        }
        log(LogLevel::INFO, "connection accepted, connection_fd: " + std::to_string(connection_fd));

        constexpr int BUFFER_SIZE = 30720;
        char buffer[BUFFER_SIZE] = {0};
        int bytes_received = read(connection_fd , buffer, BUFFER_SIZE);
        if (bytes_received < 0) {
            die("Failed to read bytes from client socket connection");
        }
        log(LogLevel::INFO, "bytes recieved from client: " + std::to_string(bytes_received));

        std::string request(buffer, bytes_received);
        std::stringstream request_stream(request);
        std::string request_line{};
        std::string first_line{};
        std::map<std::string, std::string> headers{};
        int line_count = 0;
        // read through request and store useful info for latter
        while (std::getline(request_stream, request_line)) {
            line_count++;
            if (line_count == 1) {
                first_line = request_line;
            }
            else {
                size_t found = request_line.find(":");
                if (found != std::string::npos && request_line.size() > 0) {
                    std::string header_key = trim(request_line.substr(0, found));
                    std::string header_val = trim(request_line.substr(found + 1));
                    log(LogLevel::INFO, std::format("header parsed -> {}:{}", header_key, header_val));
                    headers[header_key] = header_val;
                }
            }
        }
        // processing and printing of request
        auto [method, path, version] = process_first_line(first_line);
        log(LogLevel::INFO, std::format("HTTP method {}", to_string(method)));
        log(LogLevel::INFO, std::format("HTTP path {}", path));
        log(LogLevel::INFO, std::format("HTTP version {}", version));
        
        // sending a hardcoded response back to the client
        std::stringstream response_stream;
        response_stream << "HTTP/1.1 200 Ok\r\n";
        response_stream << "Content-Type: text/html\r\n";
        response_stream << "Connection: close\r\n\r\n";

        response_stream << "<!DOCTYPE html>";
        response_stream << "<html lang=\"en\">";
        response_stream << "<head>";
        response_stream << "  <meta charset=\"utf-8\">";
        response_stream << "  <title>Penis</title>";
        response_stream << "</head>";
        response_stream << "<body>";
        response_stream << "  <h1>";
        response_stream << path.substr(1);
        response_stream << " is GAY";
        response_stream << "  </h1>";
        response_stream << "</body>";
        response_stream << "</html>";
        int bytes_sent = write(connection_fd, response_stream.str().data(), response_stream.str().length());
        log(LogLevel::INFO, "bytes written to client: " + std::to_string(bytes_sent));
        close(connection_fd);
        log(LogLevel::INFO, std::format("connection closed: {}", connection_fd));
    }
    return 0;
}
