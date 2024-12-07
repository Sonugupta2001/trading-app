#include "MarketDataManager.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include "MarketDataManager.h"
#include "../include/Logger.h"
#include <json/json.h>

// Constructor
MarketDataManager::MarketDataManager(AuthManager& authManager) : authManager(authManager) {}

// Helper function to perform HTTP POST requests
std::string MarketDataManager::postRequest(const std::string& endpoint, const std::string& payload) {
    std::string apiUrl = "https://test.deribit.com/api/v2/" + endpoint; // Deribit Testnet API base URL
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        
        // Callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](char* ptr, size_t size, size_t nmemb, std::string* data) {
            data->append(ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Include Bearer token for authentication
        struct curl_slist* headers = nullptr;
        std::string bearerToken = "Authorization: Bearer " + authManager.getAccessToken();
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, bearerToken.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform request
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}

// Fetch the orderbook for a specific instrument
bool MarketDataManager::getOrderBook(const std::string& instrumentName) {
    // Construct API request
    std::string endpoint = "public/get_order_book";

    Json::Value request;
    request["jsonrpc"] = "2.0";
    request["id"] = 3;
    request["method"] = endpoint;

    Json::Value params;
    params["instrument_name"] = instrumentName;
    request["params"] = params;

    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, request);

    // Make the API call
    std::string response = postRequest(endpoint, payload);

    // Parse the response
    Json::CharReaderBuilder reader;
    Json::Value jsonResponse;
    std::string errs;

    std::istringstream responseStream(response);
    if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
        std::cerr << "Error parsing order book response: " << errs << std::endl;
        return false;
    }

    if (jsonResponse["result"].isObject()) {
        Json::Value result = jsonResponse["result"];

        // Display orderbook details
        std::cout << "Orderbook for " << instrumentName << ":" << std::endl;

        std::cout << "Asks:" << std::endl;
        for (const auto& ask : result["asks"]) {
            std::cout << "Price: " << ask[0].asDouble() << ", Size: " << ask[1].asDouble() << std::endl;
        }

        std::cout << "Bids:" << std::endl;
        for (const auto& bid : result["bids"]) {
            std::cout << "Price: " << bid[0].asDouble() << ", Size: " << bid[1].asDouble() << std::endl;
        }
        return true;
    } else {
        std::cerr << "Failed to retrieve order book: "
                  << jsonResponse["error"]["message"].asString() << std::endl;
        return false;
    }
}


bool MarketDataManager::getPositions(const std::string& currency) {
    // Construct API request
    std::string endpoint = "private/get_positions";

    Json::Value request;
    request["jsonrpc"] = "2.0";
    request["id"] = 4;
    request["method"] = endpoint;

    Json::Value params;
    params["currency"] = currency;
    request["params"] = params;

    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, request);

    // Make the API call
    std::string response = postRequest(endpoint, payload);

    // Parse the response
    Json::CharReaderBuilder reader;
    Json::Value jsonResponse;
    std::string errs;

    std::istringstream responseStream(response);
    if (!Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
        std::cerr << "Error parsing positions response: " << errs << std::endl;
        return false;
    }

    if (jsonResponse["result"].isArray()) {
        Json::Value positions = jsonResponse["result"];

        // Display positions
        std::cout << "Open positions for currency: " << currency << std::endl;
        for (const auto& position : positions) {
            std::cout << "Instrument: " << position["instrument_name"].asString() << std::endl;
            std::cout << "Size: " << position["size"].asDouble() << std::endl;
            std::cout << "Average Price: " << position["average_price"].asDouble() << std::endl;
            std::cout << "Direction: " << position["direction"].asString() << std::endl;
            std::cout << "-----------------------------" << std::endl;
        }

        return true;
    } else {
        std::cerr << "Failed to retrieve positions: "
                  << jsonResponse["error"]["message"].asString() << std::endl;
        return false;
    }
}


void MarketDataManager::startMarketDataStreaming(WebSocketServer& webSocketServer) {
    using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
    Client client;

    try {
        client.init_asio();
        client.set_tls_init_handler([](websocketpp::connection_hdl) {
            return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
        });

        client.set_message_handler([&webSocketServer](websocketpp::connection_hdl, Client::message_ptr msg) {
            // Parse the message and broadcast to clients
            Json::Value jsonResponse;
            std::string errs;
            std::istringstream responseStream(msg->get_payload());

            Json::CharReaderBuilder reader;
            if (Json::parseFromStream(reader, responseStream, &jsonResponse, &errs)) {
                if (jsonResponse["params"]["channel"].isString()) {
                    std::string channel = jsonResponse["params"]["channel"].asString();
                    std::string instrument = channel.substr(channel.find('.') + 1);

                    webSocketServer.broadcastToSubscribers(instrument, msg->get_payload());
                }
            }
        });

        websocketpp::lib::error_code ec;
        Client::connection_ptr con = client.get_connection("wss://test.deribit.com/ws/api/v2", ec);
        if (ec) {
            std::cerr << "Connection error: " << ec.message() << std::endl;
            return;
        }

        client.connect(con);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error in market data streaming: " << e.what() << std::endl;
    }
}

// market/MarketDataManager.cpp updates
void MarketDataManager::handleOrderBookUpdate(const std::string& instrument, const Json::Value& data) {
    std::lock_guard<std::mutex> lock(orderBooksMutex);
    
    // Get or create order book for instrument
    auto& orderBook = orderBooks[instrument];
    if (!orderBook) {
        orderBook = std::make_shared<OrderBook>(instrument);
    }

    // Update bids
    if (data.isMember("bids") && data["bids"].isArray()) {
        for (const auto& bid : data["bids"]) {
            if (bid.isArray() && bid.size() >= 2) {
                double price = bid[0].asDouble();
                double quantity = bid[1].asDouble();
                orderBook->updateBid(price, quantity);
            }
        }
    }

    // Update asks
    if (data.isMember("asks") && data["asks"].isArray()) {
        for (const auto& ask : data["asks"]) {
            if (ask.isArray() && ask.size() >= 2) {
                double price = ask[0].asDouble();
                double quantity = ask[1].asDouble();
                orderBook->updateAsk(price, quantity);
            }
        }
    }
}

void MarketDataManager::processWebSocketMessage(const std::string& message) {
    Json::CharReaderBuilder reader;
    Json::Value data;
    std::string errors;
    
    std::istringstream messageStream(message);
    if (Json::parseFromStream(reader, messageStream, &data, &errors)) {
        if (data["method"].asString() == "subscription" && 
            data["params"]["channel"].asString().find("book.") == 0) {
            
            std::string instrument = data["params"]["channel"].asString().substr(5);
            handleOrderBookUpdate(instrument, data["params"]["data"]);
        }
    }
}

std::shared_ptr<OrderBook> MarketDataManager::getOrderBook(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(orderBooksMutex);
    return orderBooks[instrument];
}

void MarketDataManager::stopMarketDataStreaming() {
    keepStreaming = false;
    if (streamingThread.joinable()) {
        streamingThread.join();
    }
}


MarketDataManager::MarketDataManager(AuthManager& authManager) 
    : authManager(authManager), 
      webSocketServer(nullptr),
      keepStreaming(false) {
}

MarketDataManager::~MarketDataManager() {
    stopMarketDataStreaming();
}

void MarketDataManager::startMarketDataStreaming(WebSocketServer& ws) {
    if (keepStreaming) {
        Logger::warning("Market data streaming is already running");
        return;
    }

    webSocketServer = &ws;
    keepStreaming = true;

    // Set up WebSocket message handler
    webSocketServer->setMessageHandler([this](const std::string& message) {
        this->processWebSocketMessage(message);
    });

    // Start streaming thread
    streamingThread = std::thread([this]() {
        try {
            streamMarketData();
        } catch (const std::exception& e) {
            Logger::error("Error in market data streaming thread: ", e.what());
            keepStreaming = false;
        }
    });

    Logger::info("Market data streaming started");
}

void MarketDataManager::streamMarketData() {
    while (keepStreaming) {
        try {
            // Ensure authentication is valid
            if (!authManager.refreshToken()) {
                Logger::error("Failed to refresh authentication token");
                break;
            }

            // Create subscription message for each instrument
            Json::Value subscriptionMsg;
            subscriptionMsg["jsonrpc"] = "2.0";
            subscriptionMsg["id"] = 123;
            subscriptionMsg["method"] = "public/subscribe";

            Json::Value channels(Json::arrayValue);
            {
                std::lock_guard<std::mutex> lock(orderBooksMutex);
                for (const auto& [instrument, _] : orderBooks) {
                    channels.append("book." + instrument + ".100ms"); // 100ms updates
                }
            }

            subscriptionMsg["params"]["channels"] = channels;

            // Send subscription message through WebSocket
            if (webSocketServer) {
                Json::StreamWriterBuilder writer;
                std::string payload = Json::writeString(writer, subscriptionMsg);
                webSocketServer->broadcastToSubscribers("", payload); // Empty string broadcasts to all
            }

            // Sleep for a short duration before next update
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        } catch (const std::exception& e) {
            Logger::error("Error in market data stream: ", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void MarketDataManager::subscribeToInstrument(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(orderBooksMutex);
    if (!orderBooks[instrument]) {
        orderBooks[instrument] = std::make_shared<OrderBook>(instrument);
        Logger::info("Created new order book for instrument: ", instrument);
        
        // If streaming is active, send subscription request
        if (keepStreaming && webSocketServer) {
            Json::Value subscriptionMsg;
            subscriptionMsg["jsonrpc"] = "2.0";
            subscriptionMsg["method"] = "public/subscribe";
            subscriptionMsg["params"]["channels"].append("book." + instrument + ".100ms");
            
            Json::StreamWriterBuilder writer;
            std::string payload = Json::writeString(writer, subscriptionMsg);
            webSocketServer->broadcastToSubscribers(instrument, payload);
        }
    }
}

void MarketDataManager::unsubscribeFromInstrument(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(orderBooksMutex);
    orderBooks.erase(instrument);
    Logger::info("Removed order book for instrument: ", instrument);
    
    if (keepStreaming && webSocketServer) {
        Json::Value unsubscribeMsg;
        unsubscribeMsg["jsonrpc"] = "2.0";
        unsubscribeMsg["method"] = "public/unsubscribe";
        unsubscribeMsg["params"]["channels"].append("book." + instrument + ".100ms");
        
        Json::StreamWriterBuilder writer;
        std::string payload = Json::writeString(writer, unsubscribeMsg);
        webSocketServer->broadcastToSubscribers(instrument, payload);
    }
}