#include <iostream>
#include <cstring> // for std::strerror
#include <unistd.h>
#include <sstream>

#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in


void die(const std::string& msg) {
    std::cerr << std::strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        die("2 cmd args reqired | <address> <port>");
    }
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        die("socket");
    }
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
    if (listen(socket_fd, 5)) {
        die("listen");
    }
    sockaddr_in connected_address{};
    socklen_t connected_address_size{};
    int connection_fd = accept(socket_fd, (sockaddr *)&connected_address, &connected_address_size);
    if (connection_fd < 0) {
        die("accept");
    }

    // std::cout << inet_ntoa(connected_address.sin_addr) << "\n";
    // std::cout << ntohs(connected_address.sin_port) << "\n";

    constexpr int BUFFER_SIZE = 30720;
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received = read(connection_fd , buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        die("Failed to read bytes from client socket connection");
    }
    std::cout << "bytes recieved from client: " << bytes_received << "\n";
    std::string request(buffer, bytes_received);
    std::stringstream request_stream(request);
    std::string request_line{};
    int line_count = 0;
    while (std::getline(request_stream, request_line)) {
        line_count++;
        std::cout << "line count: " << line_count << "\n";
        std::cout << request_line << "\n";
    }
    
    // sending a hardcoded response back to the client
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 200 Ok\r\n";
    response_stream << "Content-Type: text/html\r\n\r\n";
    response_stream << "<!DOCTYPE html>";
    response_stream << "<html lang=\"en\">";
    response_stream << "<head>";
    response_stream << "  <meta charset=\"utf-8\">";
    response_stream << "  <title>A simple webpage</title>";
    response_stream << "</head>";
    response_stream << "<body>";
    response_stream << "  <h1>Simple HTML webpage</h1>";
    response_stream << "  <p>Hello, world!</p>";
    response_stream << "</body>";
    response_stream << "</html>";
    int bytes_written = write(connection_fd, response_stream.str().data(), response_stream.str().length());
    return 0;
}
