#include <iostream>
#include <cstring> // for std::strerror
#include <unistd.h>
#include <sstream>
#include <map>
#include <tuple>
#include <vector>
#include <optional>
#include <format>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in
#include <sys/stat.h>
#include <signal.h>

#include "log.h"

void die(const std::string& msg) {
    log(LogLevel::ERROR, std::strerror(errno) + std::format(": {}", msg));
    std::exit(EXIT_FAILURE);
}

bool is_valid_path(const std::string& path) {
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0) {
        return true;
    }
    return false;
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
    GET,
    POST
};

std::string to_string(HttpMethod m) {
    switch (m) {
        case HttpMethod::GET:
            return "GET";
            break;
        case HttpMethod::POST:
            return "POST";
            break;
    }
    return "";
}

std::ostream& operator<<(std::ostream & os, HttpMethod m) {
    os << to_string(m);
    return os;
}

 auto process_first_line(std::string first_line) -> std::optional<std::tuple<HttpMethod, std::string, std::string>> {
    HttpMethod method = HttpMethod::GET;
    std::stringstream ss{first_line};
    std::string line{};
    std::vector<std::string> parameters;
    while (getline(ss, line, ' ')) {
        parameters.push_back(line);
    }
    if (parameters.size() >= 1) {
        if (trim(parameters[0]) == "GET") {
            method = HttpMethod::GET;
        }
        else if (trim(parameters[0]) == "POST") {
            method = HttpMethod::POST;
        }
        else {
            log(LogLevel::ERROR, "unknowm http method found: " + trim(parameters[0]));
            return {};
        }
        switch (method) {
            case HttpMethod::POST:
                log(LogLevel::DETAIL, first_line);
                if (parameters.size() >= 3) {
                    std::string path = parameters[1];
                    std::string version = parameters[2];
                    return std::make_optional<std::tuple<HttpMethod, std::string, std::string>>({method, path, version});
                }
                log(LogLevel::ERROR, "process_first_line had less that 3 parameters which does not work for HTTP POST: ");
                return {};
                break;
            case HttpMethod::GET:
                if (parameters.size() >= 3) {
                    std::string path = parameters[1];
                    std::string version = parameters[2];
                    return std::make_optional<std::tuple<HttpMethod, std::string, std::string>>({method, path, version});
                }
                log(LogLevel::ERROR, "process_first_line had less that 3 parameters which does not work for HTTP GET: ");
                return {};
                break;
        }
    }
    else {
        log(LogLevel::ERROR, "process_first_line had 0 parameters");
    }
    return {};
}

typedef std::string path_t;
typedef std::map<std::string, std::string> parameter_map_t;

auto disect_parameters(std::string url_params) -> parameter_map_t {
    parameter_map_t result{};
    std::stringstream ss(url_params);
    std::string param{};
    while(std::getline(ss, param, '&')) {
        size_t index_of_eq = param.find("=");
        if (index_of_eq != std::string::npos) {
            std::string key = param.substr(0, index_of_eq);
            std::string val = param.substr(index_of_eq + 1);
            log(LogLevel::DETAIL, "url param found, Key: " + key + ", Val: " + val);
            result.insert({key, val});
        }
    }
    return result;
}

// @brief parse a URL into its possible fields
// @doc https://developer.mozilla.org/en-US/docs/Learn/Common_questions/Web_mechanics/What_is_a_URL
auto disect_url(std::string url)
    -> std::tuple<path_t, std::optional<parameter_map_t>>
{
    size_t index_of_parameters = url.find("?");
    if (index_of_parameters == std::string::npos) {
        return {url, {}};
    }
    else {
        return {url.substr(0, index_of_parameters), disect_parameters(url.substr(index_of_parameters + 1))};
    }
    return {"/", {}};
}

std::string translate_resource_path(const std::string& resource_path) {
    if (resource_path == "/") {
        return "/index.html";
    }
    return resource_path;
}

enum class FileType {
    HTML,
    CSS,
    WEBP,
    JPG,
    JS
};

std::string to_string(FileType f) {
    switch (f) {
        case FileType::HTML:
            return "text/html";
            break;
        case FileType::CSS:
            return "text/css";
            break;
        case FileType::JS:
            return "text/javascript";
            break;
        case FileType::WEBP:
            return "image/webp";
            break;
        case FileType::JPG:
            return "image/jpg";
            break;
    }
    return "";
}

FileType to_file_type(std::string file_extension) {
    if (file_extension == "html") {
        return FileType::HTML;
    }
    if (file_extension == "css") {
        return FileType::CSS;
    }
    if (file_extension == "js") {
        return FileType::JS;
    }
    if (file_extension == "webp") {
        return FileType::WEBP;
    }
    if (file_extension == "jpg") {
        return FileType::JPG;
    }
    return FileType::HTML;
}

void handle_http_get_request(
    int fd, 
    const std::string& mount_dir,
    const std::string & url, 
    const std::map<std::string, std::string>& headers
) 
{
    auto [resource_path, parameters_opt] = disect_url(url);
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

void send_http_header(int fd, int http_response_status_code, const std::string& http_response_string, std::optional<std::string> body = {}) {
    std::stringstream response_stream;
    response_stream << std::format("HTTP/1.1 {} {}\r\n", http_response_status_code, http_response_string);
    response_stream << "Connection: close\r\n\r\n";
    if (body.has_value()) {
        response_stream << body.value();
    }
    std::cout << response_stream.str() << "\n";
    int bytes_sent = send(fd, response_stream.str().data(), response_stream.str().length(), 0);
    if (bytes_sent < 0) {
        die("send");
    }
    log(LogLevel::DETAIL, "send_http_header write() returned: " + std::to_string(bytes_sent));
}

void handle_stop_python_process_sigterm(int a) {
    log(LogLevel::DETAIL, "inside handle_stop_python_process_sigterm: " + std::to_string(getpid()));
    exit(0);
}

void handle_http_post_request(
    int fd, 
    const std::string& mount_dir,
    const std::string & url, 
    const std::map<std::string, std::string>& headers,
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
        send_http_header(fd, 500, "Internal Server Error");
        return;
    }
    if (pipe(ptos_pipe) == -1)
    {
        log(LogLevel::WARNING, "ptos pipe creation failed");
        send_http_header(fd, 500, "Internal Server Error");
        return;
    }

    // Forking the process
    int python_process_pid = fork();
    if (python_process_pid == -1) // fork error
    {
        log(LogLevel::WARNING, "Could not fork the python_process_pid");
        send_http_header(fd, 500, "Internal Server Error");
        return;
    }
    else if (python_process_pid == 0) // python child process
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
        send_http_header(fd, 500, "Internal Server Error");
        return;
    }

    // Forking the auto terminate process
    // int stop_python_process_pid = fork();
    // if (stop_python_process_pid == -1) // fork error
    // {
    //     log(LogLevel::WARNING, "Could not fork the stop_python_process_pid");
    //     send_http_header(fd, 500, "Internal Server Error");
    //     kill(python_process_pid, SIGTERM);
    //     return;
    // }
    // else if (stop_python_process_pid == 0) // stop python child process
    // {
    //     sleep(3);
    //     std::string command = "if ps -p " + std::to_string(python_process_pid) 
    //         + " > /dev/null; then kill -19 " + std::to_string(python_process_pid) + ";fi" ;
    //     system(command.c_str());
    //     log(LogLevel::DETAIL, "exiting stop python process");
    //     exit(0);
    // }
    
    // parent process
    log(LogLevel::DETAIL, "waitpid for python child process");
    int status{0};
    waitpid(python_process_pid, &status, 0);
    // kill the stop python process because the python process exited
    // std::string kill_stop_python_process = "if ps -p " + std::to_string(stop_python_process_pid) 
    //         + "; then kill -19 " + std::to_string(stop_python_process_pid) + ";fi" ;
    // log(LogLevel::DETAIL, kill_stop_python_process);
    // system(kill_stop_python_process.c_str());

    // log(LogLevel::DETAIL, "after kill stop python process");

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
    if (rv == -1)
        log(LogLevel::ERROR, "select failed");
    else if (rv == 0)
        log(LogLevel::WARNING, "read from python timed out");
    else
        bytes_read_from_python = read(ptos_pipe[0], &python_output_buffer, buffer_size); /* there was data to read */

    if (bytes_read_from_python == -1) {
        log(LogLevel::ERROR, "Could not read ptos pipe");
        send_http_header(fd, 500, "Internal Server Error");
        return;
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
        send_http_header(fd, 200, "Ok", python_output);
        log(LogLevel::DETAIL, "python exit: " + std::to_string(python_exit_code));
    }
    else {
        send_http_header(fd, 500, "Internal Server Error");
        log(LogLevel::WARNING, "python exit: " + std::to_string(python_exit_code));
    }
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
        size_t head_body_break = request.find("\r\n\r\n");
        std::string header = request.substr(0, head_body_break);
        std::string body = request.substr(head_body_break + 4);
        //log(LogLevel::DETAIL, std::format("full request |{}|", request));
        //log(LogLevel::DETAIL, std::format("header |{}|", header));
        //log(LogLevel::DETAIL, std::format("body |{}|", body));

        std::stringstream header_stream(header);
        std::string first_line{};
        std::getline(header_stream, first_line);
        auto opt = process_first_line(first_line);
        if (opt.has_value()) {
            std::string request_line{};
            std::map<std::string, std::string> headers{};
            while (std::getline(header_stream, request_line)) {
                size_t found = request_line.find(":");
                if (found != std::string::npos && request_line.size() > 0) {
                    std::string header_key = trim(request_line.substr(0, found));
                    std::string header_val = trim(request_line.substr(found + 1));
                    log(LogLevel::DETAIL, std::format("http header parsed -> {}:{}", header_key, header_val));
                    headers[header_key] = header_val;
                }
                else {
                    log(LogLevel::DETAIL, "request line that have no ':' :" + request_line);
                }
            }
            // processing and printing of request
            switch (std::get<0>(opt.value())) {
            case HttpMethod::GET:
                handle_http_get_request(connection_fd, mount_dir, std::get<1>(opt.value()), headers);
                break;
            case HttpMethod::POST:
                handle_http_post_request(connection_fd, mount_dir, std::get<1>(opt.value()), headers, body);
                break;
            }
            close(connection_fd);
        }
        else {
            log(LogLevel::WARNING, "first line of request has some issues");
            close(connection_fd);
            continue;
        }
    }
    return 0;
}
