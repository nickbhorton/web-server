#include <vector>

#include "http.h"
#include "log.h"
#include "url.h"


std::ostream& operator<<(std::ostream& os, const std::map<std::string, std::string> t) {
    for (auto const& item : t) {
        os << std::get<0>(item) + "->" + std::get<1>(item) << "\n";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    os << req.get_method() << "\n";
    os << req.get_path() << "\n";
    os << req.get_headers();
    os << "<--- body --->" << "\n";
    os << req.get_body() << "\n";
    return os;
}

bool HttpRequest::parse(const std::string& request) {
    size_t head_body_break = request.find("\r\n\r\n");
    std::string header_string = request.substr(0, head_body_break);
    this->body = request.substr(head_body_break + 4);

    std::stringstream header_stream(header_string);
    std::string first_line{};
    std::getline(header_stream, first_line);
    bool result = parse_first_line(first_line);
    if (!result) return false;

    std::string request_line{};
    while (std::getline(header_stream, request_line)) {
        size_t found = request_line.find(":");
        if (found != std::string::npos && request_line.size() > 0) {
            std::string header_key = trim(request_line.substr(0, found));
            std::string header_val = trim(request_line.substr(found + 1));
            headers[header_key] = header_val;
        }
    }
    return true;
}

bool HttpRequest::parse_version(const std::string& version_string) {
    // TODO by default the Version is HttpVersion::OnePointOne
    // but eventually this will actually need to be parsed
    this->version = HttpVersion::OnePointOne;
    return true;
}

bool HttpRequest::parse_method(const std::string& method_string) {
    if (trim(method_string) == "GET") {
        method = HttpMethod::GET;
        return true;
    }
    else if (trim(method_string) == "POST") {
        method = HttpMethod::POST;
        return true;
    }
    else {
        log(LogLevel::ERROR, "unknowm http method found: " + trim(method_string));
        return false;
    }
    return false;
}

bool HttpRequest::parse_first_line(const std::string& first_line) {
    std::stringstream ss{first_line};
    std::string line{};
    std::vector<std::string> parameters;
    while (getline(ss, line, ' ')) {
        parameters.push_back(line);
    }
    if (parameters.size() >= 1) {
        log(LogLevel::DETAIL, first_line);
        if (parameters.size() >= 3) {
            this->path = parameters[1];
            if (!parse_version(parameters[2])) {
                return false;
            }
            return true;
        }
        else {
            log(LogLevel::ERROR, "process_first_line had less that 3 parameters: ");
            return false;
        }
    }
    else {
        log(LogLevel::ERROR, "process_first_line had 0 parameters");
        return false;
    }
    return false;
}

// getters
HttpMethod HttpRequest::get_method() const {
    return method;
}
std::string HttpRequest::get_path() const {
    return path;
}
std::map<std::string, std::string> HttpRequest::get_headers() const {
    return headers;
}
std::string HttpRequest::get_body() const {
    if (body.has_value()) {
        return body.value();
    }
    return "";
}
