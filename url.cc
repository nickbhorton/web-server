#include <sys/stat.h>
#include <sstream>

#include "url.h"
#include "log.h"

// https://stackoverflow.com/questions/1798112/removing-leading-and-trailing-spaces-from-a-string
std::string trim(const std::string& str, const std::string& whitespace)
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

bool is_valid_path(const std::string& path) {
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0) {
        return true;
    }
    return false;
}

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

