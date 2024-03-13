#include <cstring> // for std::strerror
#include <unistd.h>
#include <sstream>
#include <map>
#include <optional>
#include <format>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in
#include <sys/stat.h>

#include "log.h"
#include "http.h"
#include "url.h"
#include "server.h"


void handle_http_get_request(
    int fd, 
    const std::string& mount_dir,
    const HttpRequest& req
) 
{
    auto [resource_path, parameters_opt] = disect_url(req.get_path());
    std::string file_path = std::format("{}{}", mount_dir, translate_resource_path(resource_path));
    if (is_valid_path(file_path)) {
        log(LogLevel::INFO, "client requesting and server sending " + file_path);
        std::string file_extension = file_path.substr(file_path.find_last_of(".") + 1);
        std::stringstream response_stream;
        response_stream << "HTTP/1.1 200 Ok\r\n";
        response_stream << "Content-Type: " << to_string(to_file_type(file_extension)) << "\r\n";
        log(LogLevel::DETAIL, "response header " + to_string(to_file_type(file_extension)));
        response_stream << "Connection: close\r\n\r\n";
        int bytes_sent = send(fd, response_stream.str().data(), response_stream.str().length(), 0);
        if (bytes_sent < 0) {
            die("send");
        }
        log(LogLevel::DETAIL, "bytes written to client: " + std::to_string(bytes_sent));
        int fdimg = open(file_path.c_str(), O_RDONLY);

        struct stat stat_buf;
        fstat(fdimg, &stat_buf);
        int img_total_size = stat_buf.st_size;
        int block_size = stat_buf.st_blksize;

        int sent_size;

        while(img_total_size > 0){
            if(img_total_size < block_size){
                sent_size = sendfile(fd, fdimg, NULL, img_total_size);            
            }
            else{
                sent_size = sendfile(fd, fdimg, NULL, block_size);
            }       
            if (sent_size < 0) {
                die("sendfile");
            }
            log(LogLevel::DETAIL, std::format("bytes of file written: {}", sent_size));
            img_total_size = img_total_size - sent_size;
        }
        close(fdimg);
        log(LogLevel::DETAIL, std::format("file transfer connection closed: {}", fd));
    }
    else {
        log(LogLevel::INFO, "client requested a file server did not find " + file_path);
        std::stringstream response_stream;
        response_stream << "HTTP/1.1 404 Not Found\r\n";
        response_stream << "Content-Type: text/html\r\n";
        response_stream << "Connection: close\r\n\r\n";
        int bytes_sent = send(fd, response_stream.str().data(), response_stream.str().length(), 0);
        if (bytes_sent < 0) {
            die("send");
        }
        log(LogLevel::DETAIL, "bytes written to client: " + std::to_string(bytes_sent));
    }
}


// if true a response was sent otherwise caller needs to send a failure response
bool handle_http_post_request(
    int fd, 
    const std::string& mount_dir,
    const std::string & url, 
    const std::map<std::string, std::string>& request_headers,
    const std::string& body
)
{
    log(LogLevel::INFO, "client POST with url: " + url);
    log(LogLevel::DETAIL, "post body: " + body);
    std::string command = std::format("python3 {}/{}.py", mount_dir, url);
    log(LogLevel::DETAIL, "command: " + command);

    int stop_pipe[2]; // fd[0] - read; fd[1] - write
    int ptos_pipe[2]; // fd[0] - read; fd[1] - write
    // Creating pipe and checking that it was created
    if (pipe(stop_pipe) == -1)
    {
        log(LogLevel::WARNING, "stop pipe creation failed");
        return false;
    }
    if (pipe(ptos_pipe) == -1)
    {
        log(LogLevel::WARNING, "ptos pipe creation failed");
        return false;
    }

    // Forking the process
    int python_process_pid = fork();
    if (python_process_pid == -1) // fork error
    {
        log(LogLevel::WARNING, "Could not fork the python_process_pid");
        return false;
    }
    // python child process
    else if (python_process_pid == 0) 
    {
        dup2(stop_pipe[0], STDIN_FILENO);
        dup2(ptos_pipe[1], STDOUT_FILENO);
        execlp("/usr/bin/python3", "/usr/bin/python3", std::format("{}/{}.py", mount_dir, url).c_str(), NULL);
        log(LogLevel::ERROR, "Failed to python child");
        exit(0);
    }
    // parent process
    // write to python process the body of the request
    if (write(stop_pipe[1], body.data(), body.size()) == -1) {
        log(LogLevel::ERROR, "Could not write into stop pipe");
        return false;
    }
    
    int status{};
    log(LogLevel::DETAIL, "waitpid for python child process");
    waitpid(python_process_pid, &status, 0);
    if (WIFEXITED(status)) {
        log(LogLevel::DETAIL, "python exited, status:" + std::to_string(WEXITSTATUS(status)));
    } else if (WIFSIGNALED(status)) {
        log(LogLevel::WARNING, "the status of the python process was TERM");
    } else if (WIFSTOPPED(status)) {
        log(LogLevel::WARNING, "the status of the python process was STOP");
    } else {
        log(LogLevel::WARNING, "the status of the python process was not TERM or STOP");
    }

    fd_set set;
    struct timeval timeout;
    int rv;
    constexpr size_t buffer_size = 1600;
    char python_output_buffer[buffer_size];

    FD_ZERO(&set); /* clear the set */
    FD_SET(ptos_pipe[0], &set); /* add our file descriptor to the set */

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    int bytes_read_from_python{};
    rv = select(ptos_pipe[0] + 1, &set, NULL, NULL, &timeout);
    if (rv == -1) {
        log(LogLevel::ERROR, "select failed");
    }
    else if (rv == 0) {
        log(LogLevel::WARNING, "read from python timed out");
    }
    else {
        bytes_read_from_python = read(ptos_pipe[0], &python_output_buffer, buffer_size); /* there was data to read */
    }

    if (bytes_read_from_python == -1) {
        log(LogLevel::ERROR, "Could not read ptos pipe");
        return false;
    }
    log(LogLevel::DETAIL, std::format("read bytes from python: {}", bytes_read_from_python));
    std::string python_output(python_output_buffer, bytes_read_from_python);
    log(LogLevel::DETAIL, std::format("python output: {}", python_output));
    int python_exit_code = WEXITSTATUS(status);
    close(stop_pipe[1]);
    close(stop_pipe[0]);
    close(ptos_pipe[1]);
    close(ptos_pipe[0]);
    if (python_exit_code == 0) {
        std::map<std::string, std::string> response_headers {{"Connection", "close"}};
        send_http_response(fd, generate_http_response(response_headers, 200, "Ok", python_output));
        log(LogLevel::DETAIL, "python exit: " + std::to_string(python_exit_code));
        return true;
    }
    else {
        log(LogLevel::WARNING, "python exit: " + std::to_string(python_exit_code));
        return false;
    }
    return false;
} 



int main(int argc, char** argv) {
    if (argc != 4) {
        die("2 cmd args reqired | <address> <port> <website mount directory>");
    }
    // setup filesystem we are mounting
    std::string mount_dir(argv[3]);
    log(LogLevel::INFO, "server mount directory: " + mount_dir);
    if (!is_valid_path(mount_dir)) {
        die("given mount path was not valid");
    }

    // create socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        die("socket");
    }
    log(LogLevel::DETAIL, "server socket: " + std::to_string(socket_fd));

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
    log(LogLevel::DETAIL, "server socket bound successfully");
    if (listen(socket_fd, 5)) {
        die("listen");
    }

    sockaddr_in connected_address{};
    socklen_t connected_address_size{};
    while (true) {
        log(LogLevel::DETAIL, "awaiting connection");
        int connection_fd = accept(socket_fd, (sockaddr *)&connected_address, &connected_address_size);
        if (connection_fd < 0) {
            die("accept");
        }
        log(LogLevel::DETAIL, "connection accepted, connection_fd: " + std::to_string(connection_fd));

        constexpr int BUFFER_SIZE = 30720;
        char buffer[BUFFER_SIZE] = {0};
        int bytes_received = recv(connection_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            die("Failed to read bytes from client socket connection");
        }
        else if (bytes_received == 0) {
            log(LogLevel::WARNING, "client closed connection");
            close(connection_fd);
            continue;
        }
        log(LogLevel::DETAIL, "bytes recieved from client: " + std::to_string(bytes_received));

        std::string request(buffer, bytes_received);
        HttpRequest req{};
        if (!req.parse(request)){
            log(LogLevel::ERROR, "failed to parse http request");
            continue;
        }
        switch (req.get_method()) {
            case HttpMethod::GET:
                handle_http_get_request(connection_fd, mount_dir, req);
                break;
            case HttpMethod::POST:
                if (!handle_http_post_request(connection_fd, mount_dir, req.get_path(), req.get_headers(), req.get_body())) {
                    std::map<std::string, std::string> response_headers {{"Connection", "close"}};
                    send_http_response(connection_fd, generate_http_response(response_headers, 500, "Internal Server Error"));
                }
                break;
        }
        close(connection_fd);
    }
    return 0;
}
