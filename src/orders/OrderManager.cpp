#include "OrderManager.h"
#include "utils/HttpClient.h"
#include <json/json.h>
#include <iostream>

OrderManager::OrderManager(AuthManager& authManager) : authManager(authManager) {}

bool OrderManager::placeOrder(const Order& order, const std::string& side) {
    if (authManager.refreshToken()) {
        std::string endpoint = "private/" + side;

        Json::Value request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = endpoint;

        Json::Value params;
        params["instrument_name"] = order.instrumentName;
        params["amount"] = order.amount;
        params["price"] = order.price;
        params["type"] = order.type;
        request["params"] = params;

        // Convert JSON to string
        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, request);

        // Make the API call
        std::string response = postRequest(endpoint, payload);

        // Parse the response
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;

        std::istringstream responseStream(response);
        if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
            std::cerr << "Error parsing order response: " << errs << std::endl;
            return false;
        }

        if (jsonResponse["result"].isObject()) {
            std::cout << "Order placed successfully. Order ID: "
                      << jsonResponse["result"]["order_id"].asString() << std::endl;
            return true;
        } else {
            std::cerr << "Order placement failed: "
                      << jsonResponse["error"]["message"].asString() << std::endl;
            return false;
        }
    }
    std::cerr << "Failed to refresh access token!" << std::endl;
    return false;
}

std::string OrderManager::postRequest(const std::string& endpoint, const std::string& payload) {
    HttpClient client;
    std::string url = baseUrl + endpoint;

    return client.post(url, payload, {
        {"Authorization", "Bearer " + authManager.getAccessToken()},
        {"Content-Type", "application/json"}
    });
}


bool OrderManager::cancelOrder(const std::string& orderId) {
    if (authManager.refreshToken()) {
        // Construct API request
        std::string endpoint = "private/cancel";

        Json::Value request;
        request["jsonrpc"] = "2.0";
        request["id"] = 2;
        request["method"] = endpoint;

        Json::Value params;
        params["order_id"] = orderId;
        request["params"] = params;

        // Convert JSON to string
        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, request);

        // Make the API call
        std::string response = postRequest(endpoint, payload);

        // Parse the response
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;

        std::istringstream responseStream(response);
        if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
            std::cerr << "Error parsing cancel order response: " << errs << std::endl;
            return false;
        }

        if (jsonResponse["result"].isObject()) {
            std::cout << "Order canceled successfully. Order ID: "
                      << jsonResponse["result"]["order_id"].asString() << std::endl;
            return true;
        } else {
            std::cerr << "Order cancellation failed: "
                      << jsonResponse["error"]["message"].asString() << std::endl;
            return false;
        }
    }
    std::cerr << "Failed to refresh access token!" << std::endl;
    return false;
}


bool OrderManager::modifyOrder(const std::string& orderId, const Order& newOrder, const std::string& side) {
    std::cout << "Modifying order with ID: " << orderId << std::endl;

    // Step 1: Cancel the existing order
    if (!cancelOrder(orderId)) {
        std::cerr << "Failed to cancel the existing order. Modification aborted." << std::endl;
        return false;
    }
    std::cout << "Order canceled successfully. Proceeding to place a new order..." << std::endl;

    // Step 2: Place a new order with updated parameters
    if (placeOrder(newOrder, side)) {
        std::cout << "Order modified successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to place the new order. Modification failed." << std::endl;
        return false;
    }
}


OrderManager::OrderManager(AuthManager& authManager) 
    : authManager(authManager),
      riskManager(std::make_unique<RiskManager>()) {
}

bool OrderManager::placeOrder(Order& order) {
    try {
        // Check rate limit
        if (!checkRateLimit()) {
            Logger::warning("Order rejected: Rate limit exceeded");
            order.status = "rejected";
            order.rejectionReason = "Rate limit exceeded";
            return false;
        }

        // Validate order through risk manager
        if (!riskManager->validateOrder(order)) {
            Logger::warning("Order rejected: Failed risk validation");
            order.status = "rejected";
            order.rejectionReason = "Failed risk validation";
            return false;
        }

        // Refresh authentication token
        if (!authManager.refreshToken()) {
            Logger::error("Failed to refresh authentication token");
            return false;
        }

        // Prepare order request
        std::string endpoint = "private/" + order.side;
        Json::Value request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = endpoint;

        Json::Value params;
        params["instrument_name"] = order.instrumentName;
        params["amount"] = order.amount;
        params["type"] = order.type;
        if (order.type == "limit") {
            params["price"] = order.price;
        }
        request["params"] = params;

        // Convert to string
        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, request);

        // Send order
        std::string response = postRequest(endpoint, payload);

        // Parse response
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;
        std::istringstream responseStream(response);

        if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
            Logger::error("Failed to parse order response: ", errs);
            return false;
        }

        // Update order status
        updateOrderStatus(order, jsonResponse);

        // If order was successful, update position
        if (order.status == "filled" || order.status == "partially_filled") {
            double filledAmount = order.side == "buy" ? order.filledAmount : -order.filledAmount;
            riskManager->updatePosition(order.instrumentName, filledAmount, order.averageFilledPrice);
        }

        // Track active order
        if (order.status != "rejected" && order.status != "cancelled") {
            std::lock_guard<std::mutex> lock(ordersMutex);
            activeOrders[order.orderId] = order;
        }

        return order.status != "rejected";

    } catch (const std::exception& e) {
        Logger::error("Error placing order: ", e.what());
        return false;
    }
}

bool OrderManager::checkRateLimit() {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::system_clock::now();
    
    // Remove requests older than 1 second
    while (!requestTimes.empty() && 
           std::chrono::duration_cast<std::chrono::seconds>(now - requestTimes.front()).count() >= 1) {
        requestTimes.pop();
    }
    
    if (requestTimes.size() >= MAX_REQUESTS_PER_SECOND) {
        return false;
    }
    
    requestTimes.push(now);
    return true;
}

void OrderManager::updateOrderStatus(Order& order, const Json::Value& response) {
    if (response["result"].isObject()) {
        order.orderId = response["result"]["order_id"].asString();
        order.status = response["result"]["order_state"].asString();
        order.filledAmount = response["result"]["filled_amount"].asDouble();
        order.averageFilledPrice = response["result"]["average_price"].asDouble();
        Logger::info("Order ", order.orderId, " status: ", order.status);
    } else {
        order.status = "rejected";
        order.rejectionReason = response["error"]["message"].asString();
        Logger::warning("Order rejected: ", order.rejectionReason);
    }
}

void OrderManager::setRiskLimits(const RiskManager::RiskLimits& limits) {
    riskManager->setRiskLimits(limits);
}

std::unordered_map<std::string, double> OrderManager::getCurrentPositions() const {
    return riskManager->getCurrentPositions();
}