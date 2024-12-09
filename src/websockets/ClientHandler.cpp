#include "ClientHandler.h"
#include <iostream>
#include <json/json.h>
#include "MarketDataManager.h"
#include "WebSocketServer.h"

ClientHandler::ClientHandler(MarketDataManager& marketDataManager, WebSocketServer& webSocketServer)
    : marketDataManager(marketDataManager), webSocketServer(webSocketServer) {}

ClientHandler::~ClientHandler() {}

void ClientHandler::onMessage(websocketpp::connection_hdl hdl, const std::string& message) {
    std::cout << "Received message from client: " << message << std::endl;

    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream iss(message);

    if (Json::parseFromStream(reader, iss, &root, &errors)) {
        std::string type = root["type"].asString();
        std::string symbol = root["symbol"].asString();

        if (type == "subscribe") {
            subscribeToMarketData(hdl, symbol);
        } else if (type == "unsubscribe") {
            unsubscribeFromMarketData(hdl, symbol);
        }
    } else {
        std::cerr << "Failed to parse message: " << errors << std::endl;
    }
}

void ClientHandler::subscribeToMarketData(websocketpp::connection_hdl hdl, const std::string& symbol) {
    std::cout << "Subscribing to market data for symbol: " << symbol << std::endl;
    marketDataManager.subscribeToInstrument(symbol);
    webSocketServer.addSubscription(hdl, symbol);
}

void ClientHandler::unsubscribeFromMarketData(websocketpp::connection_hdl hdl, const std::string& symbol) {
    std::cout << "Unsubscribing from market data for symbol: " << symbol << std::endl;
    marketDataManager.unsubscribeFromInstrument(symbol);
    webSocketServer.removeSubscription(hdl, symbol);
}

void ClientHandler::sendMarketData(websocketpp::connection_hdl hdl, const std::string& data) {
    std::cout << "Sending market data: " << data << std::endl;
    webSocketServer.send(hdl, data, websocketpp::frame::opcode::text);
}