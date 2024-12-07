#include "ClientHandler.h"
#include <iostream>

ClientHandler::ClientHandler() {}

ClientHandler::~ClientHandler() {}

void ClientHandler::onMessage(websocketpp::connection_hdl hdl, const std::string& message) {
    std::cout << "Received message from client: " << message << std::endl;
    
    // Process the message (e.g., subscribe to market data, etc.)
    // You can call `subscribeToMarketData()` or similar based on the message type
}

void ClientHandler::subscribeToMarketData(const std::string& symbol) {
    std::cout << "Subscribed to market data for symbol: " << symbol << std::endl;
    // Add the logic to handle subscription to market data
}

void ClientHandler::sendMarketData(websocketpp::connection_hdl hdl, const std::string& data) {
    // Send market data to the client
    std::cout << "Sending market data: " << data << std::endl;
}