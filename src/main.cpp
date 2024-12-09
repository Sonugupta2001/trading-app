#include <iostream>
#include <thread>
#include "auth/AuthManager.h"
#include "market/MarketDataManager.h"
#include "orders/OrderManager.h"
#include "websocket/WebSocketServer.h"
#include "include/Logger.h"

// Function prototypes for CLI options
void showMenu();
void placeOrder(OrderManager& orderManager);
void cancelOrder(OrderManager& orderManager);
void modifyOrder(OrderManager& orderManager);
void viewOrderBook(MarketDataManager& marketDataManager);
void viewPositions(MarketDataManager& marketDataManager);

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

        // Start WebSocket server in a separate thread
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

        Logger::info("Initializing order manager...");
        OrderManager orderManager(authManager);

        // CLI Loop
        int choice = 0;
        while (choice != 6) {
            showMenu();
            std::cin >> choice;

            switch (choice) {
                case 1:
                    placeOrder(orderManager);
                    break;
                case 2:
                    cancelOrder(orderManager);
                    break;
                case 3:
                    modifyOrder(orderManager);
                    break;
                case 4:
                    viewOrderBook(marketDataManager);
                    break;
                case 5:
                    viewPositions(marketDataManager);
                    break;
                case 6:
                    Logger::info("Exiting the system. Goodbye!");
                    break;
                default:
                    std::cout << "Invalid choice. Please try again.\n";
            }
        }

        // Wait for WebSocket thread to finish
        wsThread.join();
        Logger::info("Trading system shutting down normally");
        return 0;

    } catch (const std::exception& e) {
        Logger::fatal("Fatal error in trading system: ", e.what());
        return 1;
    }
}

// CLI Menu Display
void showMenu() {
    std::cout << "\n===== GoQuant CLI =====\n";
    std::cout << "1. Place an Order\n";
    std::cout << "2. Cancel an Order\n";
    std::cout << "3. Modify an Order\n";
    std::cout << "4. View Order Book\n";
    std::cout << "5. View Current Positions\n";
    std::cout << "6. Exit\n";
    std::cout << "Enter your choice: ";
}

// CLI Options Implementation
void placeOrder(OrderManager& orderManager) {
    Order order;
    std::cout << "Enter instrument name (e.g., BTC-PERPETUAL): ";
    std::cin >> order.instrumentName;
    std::cout << "Enter side (buy/sell): ";
    std::cin >> order.side;
    std::cout << "Enter amount: ";
    std::cin >> order.amount;
    std::cout << "Enter type (market/limit): ";
    std::cin >> order.type;
    if (order.type == "limit") {
        std::cout << "Enter price: ";
        std::cin >> order.price;
    }

    if (orderManager.placeOrder(order)) {
        std::cout << "Order placed successfully!\n";
    } else {
        std::cout << "Failed to place order.\n";
    }
}

void cancelOrder(OrderManager& orderManager) {
    std::string orderId;
    std::cout << "Enter order ID to cancel: ";
    std::cin >> orderId;

    if (orderManager.cancelOrder(orderId)) {
        std::cout << "Order canceled successfully!\n";
    } else {
        std::cout << "Failed to cancel order.\n";
    }
}

void modifyOrder(OrderManager& orderManager) {
    std::string orderId;
    Order newOrder;
    std::cout << "Enter order ID to modify: ";
    std::cin >> orderId;

    std::cout << "Enter new amount: ";
    std::cin >> newOrder.amount;
    std::cout << "Enter new price: ";
    std::cin >> newOrder.price;

    if (orderManager.modifyOrder(orderId, newOrder)) {
        std::cout << "Order modified successfully!\n";
    } else {
        std::cout << "Failed to modify order.\n";
    }
}

void viewOrderBook(MarketDataManager& marketDataManager) {
    std::string instrument;
    std::cout << "Enter instrument name (e.g., BTC-PERPETUAL): ";
    std::cin >> instrument;

    if (!marketDataManager.getOrderBook(instrument)) {
        std::cout << "Failed to fetch order book.\n";
    }
}

void viewPositions(MarketDataManager& marketDataManager) {
    std::string currency;
    std::cout << "Enter currency (e.g., BTC): ";
    std::cin >> currency;

    if (!marketDataManager.getPositions(currency)) {
        std::cout << "Failed to fetch positions.\n";
    }
}