#include "ExecutionManager.h"
#include "../include/Logger.h"
#include "utils/HttpClient.h"
#include <json/json.h>
#include <chrono>

ExecutionManager::ExecutionManager(AuthManager& authManager) : running(false), authManager(authManager) {}

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
        auto start = std::chrono::high_resolution_clock::now(); // Start time
        // Prepare API request for placing the order
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

        // Convert JSON to string
        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, request);

        // Send the request using HttpClient
        HttpClient client;
        std::string response = client.post("https://test.deribit.com/api/v2/" + endpoint, payload, {
            {"Authorization", "Bearer " + authManager.getAccessToken()},
            {"Content-Type", "application/json"}
        });

        // Parse the response
        Json::CharReaderBuilder reader;
        Json::Value jsonResponse;
        std::string errs;
        std::istringstream responseStream(response);
        if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
            Logger::error("Failed to parse order response: ", errs);
            order.status = "failed";
            return;
        }

        // Update order status based on response
        updateOrderStatus(order, jsonResponse);

        // Process fills if the order was filled
        if (order.status == "filled" || order.status == "partially_filled") {
            double filledAmount = order.side == "buy" ? order.filledAmount : -order.filledAmount;
            riskManager->updatePosition(order.instrumentName, filledAmount, order.averageFilledPrice);
        }

        auto end = std::chrono::high_resolution_clock::now(); // End time
        std::chrono::duration<double, std::milli> latency = end - start;
        Logger::info("Order placement latency: ", latency.count(), " ms");

    }
    catch (const std::exception& e) {
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