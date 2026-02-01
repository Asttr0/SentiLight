#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body; 
};

// Helper: Convert string to lowercase
inline std::string to_lower_http(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

inline HttpRequest parse_request(std::string raw_request) {
    HttpRequest req;
    
    // 1. Split Header Section and Body Section
    // HTTP standards separate headers and body with a double newline (\r\n\r\n)
    size_t header_end = raw_request.find("\r\n\r\n");
    std::string header_part;
    
    if (header_end != std::string::npos) {
        header_part = raw_request.substr(0, header_end);
        req.body = raw_request.substr(header_end + 4); // Capture the Body!
    } else {
        header_part = raw_request;
    }

    std::istringstream stream(header_part);
    std::string line;

    // 2. Parse Request Line (GET /path HTTP/1.1)
    if (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream line_stream(line);
        line_stream >> req.method >> req.path >> req.version;
    }

    // 3. Parse Headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;

        size_t delimiter_pos = line.find(":");
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            
            // Trim leading space in value
            if (!value.empty() && value[0] == ' ') value.erase(0, 1);
            
            // Normalize Key to Lowercase 
            req.headers[to_lower_http(key)] = value;
        }
    }
    return req;
}

#endif