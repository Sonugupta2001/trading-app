#include <iostream>
#include <thread>
#include "auth/AuthManager.h"
#include "market/MarketDataManager.h"
#include "websocket/WebSocketServer.h"
#include "include/Logger.h"

int main() {
    try {
        Logger::init("trading_system.log");
        Logger::setLogLevel(Logger::INFO);
        Logger::info("Trading system starting up...");

        Logger::info("Loading configuration...");
        std::string clientId = "i4jalK_j";
        std::string clientSecret = "xaHhTgT_w21ZINwPivwoxaZ0VRpqLusSo-0SL9TFCMs";
        
        Logger::info("Initializing authentication manager...");
        AuthManager authManager(clientId, clientSecret);
        
        if (!authManager.authenticate()) {
            Logger::fatal("Authentication failed! Unable to proceed.");
            return 1;
        }
        Logger::info("Authentication successful");

        Logger::info("Initializing WebSocket server...");
        WebSocketServer webSocketServer;
        

        std::thread wsThread([&]() {
            try {
                Logger::info("Starting WebSocket server on port 9002");
                webSocketServer.run(9002);
            } catch (const std::exception& e) {
                Logger::error("WebSocket server error: ", e.what());
                throw;
            }
        });


        Logger::info("Initializing market data manager...");
        MarketDataManager marketDataManager(authManager);
        
        try {
            Logger::info("Starting market data streaming...");
            marketDataManager.startMarketDataStreaming(webSocketServer);
        } catch (const std::exception& e) {
            Logger::error("Market data streaming error: ", e.what());
            throw;
        }


        wsThread.join();
        Logger::info("Trading system shutting down normally");
        return 0;

    }
    catch (const std::exception& e) {
        Logger::fatal("Fatal error in trading system: ", e.what());
        return 1;
    }
}