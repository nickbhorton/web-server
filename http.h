#ifndef HTTP_HEADER
#define HTTP_HEADER

#include <iostream>
#include <map>
#include <optional>
#include <sstream>

enum class HttpMethod {
    GET,
    //HEAD,
    POST,
    //PUT,
    //DELETE,
    //CONNECT,
    //OPTIONS,
    //TRACE,
    //PATCH
};

enum class HttpVersion {
    OnePointOne,
    Two,
    Three,
};

// from http_method.cc
std::string to_string(HttpMethod m);
std::ostream& operator<<(std::ostream & os, HttpMethod m);
//

class HttpMessage {
public:
    HttpVersion version;
    std::map<std::string, std::string> headers;
    std::optional<std::string> body;

    HttpMessage(const HttpMessage& other) = delete;
    HttpMessage(HttpMessage&& other) = delete;
    HttpMessage& operator=(const HttpMessage& other) = delete;
    HttpMessage& operator=(HttpMessage&& other) = delete;
    HttpMessage() :
        version{HttpVersion::OnePointOne},
        headers{},
        body{}
    {}
};

std::string generate_http_response(
    const std::map<std::string, std::string> headers,
    unsigned int response_status_code = 200, 
    const std::string& response_string = "Ok",
    std::optional<std::string> body = {},
    const std::string& version = "1.1"
); 
void send_http_response(int fd, const std::string& response);

class HttpResponse : private HttpMessage {
    unsigned int status_code;
    std::string status_message;

    HttpResponse(const HttpResponse& other) = delete;
    HttpResponse(HttpResponse&& other) = delete;
    HttpResponse& operator=(const HttpResponse& other) = delete;
    HttpResponse& operator=(HttpResponse&& other) = delete;

public:
    HttpResponse() :
        status_code{},
        status_message{}
    {}

    // getters
    std::string get_status_message();
    unsigned int get_status_code();
};

class HttpRequest : private HttpMessage {
    HttpMethod method;
    std::string path;

    // Deleteing all move and copy stuff at the moment
    HttpRequest(const HttpRequest& other) = delete;
    HttpRequest(HttpRequest&& other) = delete;
    HttpRequest& operator=(const HttpRequest& other) = delete;
    HttpRequest& operator=(HttpRequest&& other) = delete;

public:
    HttpRequest() : 
        method{HttpMethod::GET},
        path{}
    {}
    
    bool parse_version(const std::string& version_string);
    bool parse_method(const std::string& method_string);
    bool parse_first_line(const std::string&);

    // use this function
    bool parse(const std::string& request);

    // getters
    HttpMethod get_method() const;
    std::string get_path() const;
    std::map<std::string, std::string> get_headers() const;
    std::string get_body() const;
};

std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

#endif
