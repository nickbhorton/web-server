#include <iostream>
#include <cstring> // for std::strerror

#include <sys/socket.h>

void die(const std::string& msg) {
    std::cerr << std::strerror(errno) << "\n";
    std::exit(EXIT_FAILURE);
}

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        die("socket");
    }
    std::cout << socket_fd << "\n";
    return 0;
}
