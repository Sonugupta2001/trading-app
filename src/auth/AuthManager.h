#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <string>
#include <mutex>

class AuthManager {
public:
    AuthManager(const std::string& clientId, const std::string& clientSecret, const std::string& apiUrl);

    bool authenticate();
    bool refreshToken();
    std::string getAccessToken();

private:
    const std::string clientId;
    const std::string clientSecret;
    const std::string apiUrl;
    std::string accessToken;
    std::string refreshToken;
    std::mutex tokenMutex;

    bool makeAuthRequest(const std::string& grantType, const std::string& token = "");
};

#endif // AUTHMANAGER_H
