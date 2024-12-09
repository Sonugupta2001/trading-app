#include "ExecutionManager.h"
#include "../include/Logger.h"

ExecutionManager::ExecutionManager() : running(false) {}

ExecutionManager::~ExecutionManager() {
    stop();
}

void ExecutionManager::start() {
    running = true;
    executionThread = std::thread(&ExecutionManager::executionLoop, this);
    Logger::info("Execution manager started");
}

void ExecutionManager::stop() {
    if (running) {
        running = false;
        queueCV.notify_one();
        if (executionThread.joinable()) {
            executionThread.join();
        }
        Logger::info("Execution manager stopped");
    }
}

void ExecutionManager::addOrder(Order& order) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        orderQueue.push(order);
    }
    queueCV.notify_one();
    Logger::info("Order added to execution queue: ", order.orderId);
}

void ExecutionManager::setFillCallback(FillCallback callback) {
    fillCallback = callback;
}

void ExecutionManager::executionLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCV.wait(lock, [this]() {
            return !orderQueue.empty() || !running;
        });

        if (!running) break;

        while (!orderQueue.empty()) {
            Order order = orderQueue.front();
            orderQueue.pop();
            lock.unlock();

            handleExecution(order);

            lock.lock();
        }
    }
}

void ExecutionManager::handleExecution(Order& order) {
    try {
        // Simulate order execution (replace with actual exchange interaction)
        order.status = "executing";
        
        // For market orders, execute immediately
        if (order.type == "market") {
            Fill fill;
            fill.orderId = order.orderId;
            fill.amount = order.amount;
            fill.price = order.price; // In real system, would be market price
            fill.timestamp = std::chrono::system_clock::now();

            std::lock_guard<std::mutex> lock(queueMutex);
            fillQueue.push(fill);
            
            Logger::info("Market order executed: ", order.orderId);
        }
        // For limit orders, check price conditions
        else if (order.type == "limit") {
            // In real system, would check against market price
            bool priceConditionMet = true; // Placeholder
            
            if (priceConditionMet) {
                Fill fill;
                fill.orderId = order.orderId;
                fill.amount = order.amount;
                fill.price = order.price;
                fill.timestamp = std::chrono::system_clock::now();

                std::lock_guard<std::mutex> lock(queueMutex);
                fillQueue.push(fill);
                
                Logger::info("Limit order executed: ", order.orderId);
            }
        }

        processFills();

    } catch (const std::exception& e) {
        Logger::error("Error executing order ", order.orderId, ": ", e.what());
        order.status = "failed";
    }
}

void ExecutionManager::processFills() {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    while (!fillQueue.empty()) {
        Fill& fill = fillQueue.front();
        
        if (fillCallback) {
            Order filledOrder;
            filledOrder.orderId = fill.orderId;
            filledOrder.status = "filled";
            filledOrder.filledAmount = fill.amount;
            filledOrder.averageFilledPrice = fill.price;
            
            fillCallback(filledOrder, fill.amount, fill.price);
        }
        
        fillQueue.pop();
    }
}