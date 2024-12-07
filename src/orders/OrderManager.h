#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "auth/AuthManager.h"
#include "models/Order.h"
#include "risk/RiskManager.h"
#include <string>
#include <memory>
#include <queue>

class OrderManager {
public:
    explicit OrderManager(AuthManager& authManager);

    OrderManager::OrderManager(
    AuthManager& authManager, 
    MarketDataManager& marketDataManager)
    : authManager(authManager),
      riskManager(std::make_unique<RiskManager>()),
      executionManager(std::make_unique<ExecutionManager>()),
      marketDataIntegrator(std::make_unique<MarketDataIntegrator>(
          marketDataManager, *executionManager)) {
    
    executionManager->setFillCallback([this](const Order& order, double amount, double price) {
        this->onFill(order, amount, price);
    });
    
    executionManager->start();
    marketDataIntegrator->start();
}

    // Order management methods
    bool placeOrder(Order& order);
    bool cancelOrder(const std::string& orderId);
    bool modifyOrder(const std::string& orderId, Order& newOrder);
    
    // Risk management methods
    void setRiskLimits(const RiskManager::RiskLimits& limits);
    std::unordered_map<std::string, double> getCurrentPositions() const;

private:
    AuthManager& authManager;
    std::unique_ptr<RiskManager> riskManager;
    const std::string baseUrl = "https://test.deribit.com/api/v2/";

    std::unique_ptr<MarketDataIntegrator> marketDataIntegrator;
    
    // Order tracking
    std::unordered_map<std::string, Order> activeOrders;
    std::mutex ordersMutex;
    
    // Rate limiting
    std::queue<std::chrono::system_clock::time_point> requestTimes;
    std::mutex rateLimitMutex;
    const int MAX_REQUESTS_PER_SECOND = 10;

    std::string postRequest(const std::string& endpoint, const std::string& payload);
    bool checkRateLimit();
    void updateOrderStatus(Order& order, const Json::Value& response);


private:
    std::unique_ptr<ExecutionManager> executionManager;
    void onFill(const Order& order, double amount, double price);

// In OrderManager.cpp constructor:
OrderManager::OrderManager(AuthManager& authManager) 
    : authManager(authManager),
      riskManager(std::make_unique<RiskManager>()),
      executionManager(std::make_unique<ExecutionManager>()) {
    
    executionManager->setFillCallback([this](const Order& order, double amount, double price) {
        this->onFill(order, amount, price);
    });
    executionManager->start();
}

// Add fill handler method:
void OrderManager::onFill(const Order& order, double amount, double price) {
    try {
        // Update position
        double signedAmount = order.side == "buy" ? amount : -amount;
        riskManager->updatePosition(order.instrumentName, signedAmount, price);
        
        // Update order status
        {
            std::lock_guard<std::mutex> lock(ordersMutex);
            if (activeOrders.find(order.orderId) != activeOrders.end()) {
                activeOrders[order.orderId].status = "filled";
                activeOrders[order.orderId].filledAmount = amount;
                activeOrders[order.orderId].averageFilledPrice = price;
            }
        }
        
        Logger::info("Order ", order.orderId, " filled: ", amount, " @ ", price);
    } catch (const std::exception& e) {
        Logger::error("Error processing fill for order ", order.orderId, ": ", e.what());
    }
}
};

#endif