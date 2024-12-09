#include "AuthManager.h"
#include "utils/HttpClient.h"
#include <json/json.h>
#include <iostream>
#include <chrono>

AuthManager::AuthManager(const std::string& clientId, const std::string& clientSecret, const std::string& apiUrl)
    : clientId(clientId), clientSecret(clientSecret), apiUrl(apiUrl) {}

bool AuthManager::authenticate() {
    return makeAuthRequest("client_credentials");
}

bool AuthManager::refreshToken() {
    std::lock_guard<std::mutex> lock(tokenMutex);
    return makeAuthRequest("refresh_token", refreshToken);
}

std::string AuthManager::getAccessToken() {
    std::lock_guard<std::mutex> lock(tokenMutex);
    return accessToken;
}

bool AuthManager::makeAuthRequest(const std::string& grantType, const std::string& token) {
    Json::Value requestBody;
    requestBody["jsonrpc"] = "2.0";
    requestBody["id"] = 0;
    requestBody["method"] = "public/auth";
    requestBody["params"]["grant_type"] = grantType;
    requestBody["params"]["client_id"] = clientId;
    requestBody["params"]["client_secret"] = clientSecret;

    if (grantType == "refresh_token") {
        requestBody["params"]["refresh_token"] = token;
    }

    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, requestBody);

    HttpClient client;
    std::string response;
    try {
        response = client.post(apiUrl, payload, {{"Content-Type", "application/json"}});
    } catch (const std::exception& e) {
        std::cerr << "Error in HTTP request: " << e.what() << std::endl;
        return false;
    }

    Json::CharReaderBuilder reader;
    Json::Value jsonResponse;
    std::string errs;
    std::istringstream responseStream(response);
    if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
        std::cerr << "Error parsing JSON response: " << errs << std::endl;
        return false;
    }

    if (jsonResponse.isMember("error")) {
        std::cerr << "Authentication error: " << jsonResponse["error"].asString() << std::endl;
        return false;
    }

    if (jsonResponse["result"].isObject()) {
        accessToken = jsonResponse["result"]["access_token"].asString();
        if (grantType == "client_credentials") {
            refreshToken = jsonResponse["result"]["refresh_token"].asString();
        }
        return true;
    }

    std::cerr << "Unexpected response format." << std::endl;
    return false;
}