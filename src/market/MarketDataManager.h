// market/MarketDataManager.h
#ifndef MARKETDATAMANAGER_H
#define MARKETDATAMANAGER_H

#include "auth/AuthManager.h"
#include "websocket/WebSocketServer.h"
#include "models/orderBook.h"
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <memory>

class MarketDataManager {
public:
    explicit MarketDataManager(AuthManager& authManager);
    ~MarketDataManager();

    void startMarketDataStreaming(WebSocketServer& webSocketServer);
    void stopMarketDataStreaming();
    
    // New methods for order book management
    std::shared_ptr<OrderBook> getOrderBook(const std::string& instrument);
    void subscribeToInstrument(const std::string& instrument);
    void unsubscribeFromInstrument(const std::string& instrument);

private:
    AuthManager& authManager;
    WebSocketServer* webSocketServer;
    std::atomic<bool> keepStreaming;
    std::thread streamingThread;

    
    
    // Order book management
    std::unordered_map<std::string, std::shared_ptr<OrderBook>> orderBooks;
    mutable std::mutex orderBooksMutex;

    void streamMarketData();
    void handleOrderBookUpdate(const std::string& instrument, const Json::Value& data);
    void processWebSocketMessage(const std::string& message);
};

#endif