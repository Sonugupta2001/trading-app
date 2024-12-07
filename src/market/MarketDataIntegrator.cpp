// market/MarketDataIntegrator.cpp
#include "MarketDataIntegrator.h"
#include "../include/Logger.h"

MarketDataIntegrator::MarketDataIntegrator(
    MarketDataManager& mdManager, 
    ExecutionManager& execManager)
    : marketDataManager(mdManager),
      executionManager(execManager),
      running(false) {
}

void MarketDataIntegrator::start() {
    running = true;
    watchThread = std::thread(&MarketDataIntegrator::watchLoop, this);
    Logger::info("Market data integrator started");
}

void MarketDataIntegrator::stop() {
    running = false;
    if (watchThread.joinable()) {
        watchThread.join();
    }
    Logger::info("Market data integrator stopped");
}

void MarketDataIntegrator::addOrderForPriceWatch(const Order& order) {
    std::lock_guard<std::mutex> lock(watchMutex);
    OrderWatch watch{
        order,
        order.price,
        std::chrono::system_clock::now()
    };
    watchedOrders[order.orderId] = watch;
    Logger::info("Added order to price watch: ", order.orderId);
}

void MarketDataIntegrator::removeOrderFromWatch(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(watchMutex);
    watchedOrders.erase(orderId);
    Logger::info("Removed order from price watch: ", orderId);
}

void MarketDataIntegrator::watchLoop() {
    while (running) {
        try {
            checkPriceConditions();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            Logger::error("Error in market data watch loop: ", e.what());
        }
    }
}

void MarketDataIntegrator::checkPriceConditions() {
    std::lock_guard<std::mutex> lock(watchMutex);
    
    auto it = watchedOrders.begin();
    while (it != watchedOrders.end()) {
        const auto& orderId = it->first;
        const auto& watch = it->second;
        
        auto orderBook = marketDataManager.getOrderBook(watch.order.instrumentName);
        if (!orderBook) {
            ++it;
            continue;
        }
        
        double currentPrice = watch.order.side == "buy" ? 
            orderBook->getBestAsk() : orderBook->getBestBid();
            
        if (isPriceConditionMet(watch.order, currentPrice)) {
            // Update order price to current market price
            Order executionOrder = watch.order;
            executionOrder.price = currentPrice;
            
            // Send to execution
            executionManager.addOrder(executionOrder);
            Logger::info("Price condition met for order: ", orderId, 
                        " at price: ", currentPrice);
            
            // Remove from watch
            it = watchedOrders.erase(it);
        } else {
            ++it;
        }
    }
}

bool MarketDataIntegrator::isPriceConditionMet(const Order& order, double currentPrice) {
    if (order.type == "market") {
        return true;
    }
    
    if (order.side == "buy") {
        return currentPrice <= order.price;
    } else {
        return currentPrice >= order.price;
    }
}