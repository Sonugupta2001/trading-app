#ifndef EXECUTION_MANAGER_H
#define EXECUTION_MANAGER_H

#include "../orders/models/Order.h"
#include "../auth/AuthManager.h"
#include <queue>
#include <functional>
#include <thread>
#include <atomic>

class ExecutionManager {
public:
    using FillCallback = std::function<void(const Order&, double, double)>;

    ExecutionManager(AuthManager& authManager);
    ~ExecutionManager();

    void start();
    void stop();
    void addOrder(Order& order);
    void setFillCallback(FillCallback callback);

private:
    AuthManager& authManager;
    struct Fill {
        std::string orderId;
        double price;
        double amount;
        std::chrono::system_clock::time_point timestamp;
    };

    std::queue<Order> orderQueue;
    std::queue<Fill> fillQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<bool> running;
    std::thread executionThread;
    FillCallback fillCallback;

    void executionLoop();
    void processFills();
    void handleExecution(Order& order);
};

#endif