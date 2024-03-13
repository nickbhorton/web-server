#include "http.h"

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
