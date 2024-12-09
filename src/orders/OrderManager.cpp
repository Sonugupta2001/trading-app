#include "OrderManager.h"
#include "utils/HttpClient.h"
#include <json/json.h>
#include <iostream>

OrderManager::OrderManager(AuthManager& authManager) 
    : authManager(authManager),
      riskManager(std::make_unique<RiskManager>()) {}

bool OrderManager::placeOrder(Order& order) {
    try {
        if (!checkRateLimit()) {
            Logger::warning("Order rejected: Rate limit exceeded");
            order.status = "rejected";
            order.rejectionReason = "Rate limit exceeded";
            return false;
        }

        if (!riskManager->validateOrder(order)) {
            Logger::warning("Order rejected: Failed risk validation");
            order.status = "rejected";
            order.rejectionReason = "Failed risk validation";
            return false;
        }

        if (!authManager.refreshToken()) {
            Logger::error("Failed to refresh authentication token");
            return false;
        }

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

        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, request);

        std::string response = postRequest(endpoint, payload);

        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;
        std::istringstream responseStream(response);

        if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
            Logger::error("Failed to parse order response: ", errs);
            return false;
        }

        updateOrderStatus(order, jsonResponse);

        if (order.status == "filled" || order.status == "partially_filled") {
            double filledAmount = order.side == "buy" ? order.filledAmount : -order.filledAmount;
            riskManager->updatePosition(order.instrumentName, filledAmount, order.averageFilledPrice);
        }

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

bool OrderManager::cancelOrder(const std::string& orderId) {
    if (!authManager.refreshToken()) {
        std::cerr << "Failed to refresh access token!" << std::endl;
        return false;
    }

    std::string endpoint = "private/cancel";
    Json::Value request;
    request["jsonrpc"] = "2.0";
    request["id"] = 2;
    request["method"] = endpoint;

    Json::Value params;
    params["order_id"] = orderId;
    request["params"] = params;

    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, request);

    std::string response = postRequest(endpoint, payload);

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

bool OrderManager::modifyOrder(const std::string& orderId, const Order& newOrder, const std::string& side) {
    std::cout << "Modifying order with ID: " << orderId << std::endl;

    if (!cancelOrder(orderId)) {
        std::cerr << "Failed to cancel the existing order. Modification aborted." << std::endl;
        return false;
    }
    std::cout << "Order canceled successfully. Proceeding to place a new order..." << std::endl;

    if (placeOrder(newOrder, side)) {
        std::cout << "Order modified successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to place the new order. Modification failed." << std::endl;
        return false;
    }
}

std::string OrderManager::postRequest(const std::string& endpoint, const std::string& payload) {
    HttpClient client;
    std::string url = baseUrl + endpoint;

    return client.post(url, payload, {
        {"Authorization", "Bearer " + authManager.getAccessToken()},
        {"Content-Type", "application/json"}
    });
}

bool OrderManager::checkRateLimit() {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    auto now = std::chrono::system_clock::now();
    
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