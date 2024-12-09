#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "auth/AuthManager.h"
#include "models/Order.h"
#include "risk/RiskManager.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <queue>
#include <chrono>

class OrderManager {
public:
    explicit OrderManager(AuthManager& authManager);
    OrderManager(AuthManager& authManager, MarketDataManager& marketDataManager);

    bool placeOrder(Order& order);
    bool cancelOrder(const std::string& orderId);
    bool modifyOrder(const std::string& orderId, Order& newOrder);

    void setRiskLimits(const RiskManager::RiskLimits& limits);
    std::unordered_map<std::string, double> getCurrentPositions() const;

private:
    AuthManager& authManager;
    std::unique_ptr<RiskManager> riskManager;
    std::unique_ptr<ExecutionManager> executionManager;
    std::unique_ptr<MarketDataIntegrator> marketDataIntegrator;

    const std::string baseUrl = "https://test.deribit.com/api/v2/";

    std::unordered_map<std::string, Order> activeOrders;
    std::mutex ordersMutex;

    std::queue<std::chrono::system_clock::time_point> requestTimes;
    std::mutex rateLimitMutex;
    static constexpr int MAX_REQUESTS_PER_SECOND = 10;

    std::string postRequest(const std::string& endpoint, const std::string& payload);
    bool checkRateLimit();
    void updateOrderStatus(Order& order, const Json::Value& response);
    void onFill(const Order& order, double amount, double price);
};

#endif