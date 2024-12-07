#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>
#include <map>

class HttpClient {
public:
    std::string post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers);
};

#endif
