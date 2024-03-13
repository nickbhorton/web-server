#ifndef URL_HEADER
#define URL_HEADER

#include <map>
#include <optional>
#include <string>

enum class FileType { HTML, CSS, WEBP, JPG, JS };

typedef std::string path_t;
typedef std::map<std::string, std::string> parameter_map_t;

// filetype.cc
auto to_file_type(std::string file_extension)
    -> FileType;
auto to_string(FileType f)
    -> std::string;
// url.cc
auto trim(const std::string& str, const std::string& whitespace = " \t\n")
    -> std::string;
auto is_valid_path(const std::string &path) 
    -> bool;
auto disect_parameters(std::string url_params) 
    -> parameter_map_t;
auto disect_url(std::string url)
    -> std::tuple<path_t, std::optional<parameter_map_t>>;
auto translate_resource_path(const std::string &resource_path) 
    -> std::string;

#endif
