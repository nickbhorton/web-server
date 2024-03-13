#include "url.h"

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
