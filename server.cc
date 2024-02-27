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
    std::cout << std::string(buffer, bytes_received);
    std::stringstream ss;
    ss << "HTTP/1.1 200 Ok\r\n";
    ss << "Content-Type: text/html\r\n\r\n";
    ss << "<!DOCTYPE html>";
    ss << "<html lang=\"en\">";
    ss << "<head>";
    ss << "  <meta charset=\"utf-8\">";
    ss << "  <title>A simple webpage</title>";
    ss << "</head>";
    ss << "<body>";
    ss << "  <h1>Simple HTML webpage</h1>";
    ss << "  <p>Hello, world!</p>";
    ss << "</body>";
    ss << "</html>";
    int bytes_written = write(connection_fd, ss.str().data(), ss.str().length());
    return 0;
}
