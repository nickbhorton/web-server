#include <iostream>
#include <cstring> // for std::strerror

#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in


void die(const std::string& msg) {
    std::cerr << std::strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        die("2 cmd args reqired");
    }
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        die("socket");
    }
    std::cout << "socket_fd: " << socket_fd << "\n";
    sockaddr_in server_address{};
    server_address.sin_family        = AF_INET;
    server_address.sin_port          = htons(8080);
    server_address.sin_addr.s_addr   = inet_addr(argv[1]);
    if (bind(socket_fd, (sockaddr *)&server_address, sizeof(server_address))) {
        die("bind");
    }
    return 0;
}
