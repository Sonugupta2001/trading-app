// market/MarketDataIntegrator.h
#ifndef MARKET_DATA_INTEGRATOR_H
#define MARKET_DATA_INTEGRATOR_H

#include "MarketDataManager.h"
#include "../execution/ExecutionManager.h"
#include "../orders/models/Order.h"
#include <map>
#include <mutex>

class MarketDataIntegrator {
public:
    MarketDataIntegrator(MarketDataManager& mdManager, ExecutionManager& execManager);
    
    void start();
    void stop();
    void addOrderForPriceWatch(const Order& order);
    void removeOrderFromWatch(const std::string& orderId);

private:
    MarketDataManager& marketDataManager;
    ExecutionManager& executionManager;
    std::atomic<bool> running;
    std::thread watchThread;
    
    struct OrderWatch {
        Order order;
        double targetPrice;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::map<std::string, OrderWatch> watchedOrders;
    std::mutex watchMutex;
    
    void watchLoop();
    void checkPriceConditions();
    bool isPriceConditionMet(const Order& order, double currentPrice);
};

#endif