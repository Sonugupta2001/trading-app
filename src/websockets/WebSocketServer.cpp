#include "WebSocketServer.h"
#include "../include/Logger.h"
#include <json/json.h>
#include <chrono>

WebSocketServer::WebSocketServer() : running(false) {
    server.init_asio();
    
    server.set_open_handler([this](ConnectionHandle hdl) {
        this->onOpen(hdl);
    });
    
    server.set_close_handler([this](ConnectionHandle hdl) {
        this->onClose(hdl);
    });
    
    server.set_message_handler([this](ConnectionHandle hdl, Server::message_ptr msg) {
        this->onMessage(hdl, msg);
    });

    setupTLS();
}

void WebSocketServer::setupTLS() {
    server.set_tls_init_handler([](ConnectionHandle hdl) {
        auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
        try {
            ctx->set_options(
                boost::asio::ssl::context::default_workarounds |
                boost::asio::ssl::context::no_sslv2 |
                boost::asio::ssl::context::no_sslv3 |
                boost::asio::ssl::context::single_dh_use
            );
            ctx->use_certificate_chain_file("cert.pem");
            ctx->use_private_key_file("key.pem", boost::asio::ssl::context::pem);
        } catch (std::exception& e) {
            Logger::error("TLS setup error: ", e.what());
        }
        return ctx;
    });
}

void WebSocketServer::onOpen(ConnectionHandle hdl) {
    std::lock_guard<std::mutex> lock(mutex);
    Logger::info("New WebSocket connection established");
}

void WebSocketServer::onClose(ConnectionHandle hdl) {
    std::lock_guard<std::mutex> lock(mutex);
    // Remove client from all subscriptions
    for (auto& pair : subscriptions) {
        pair.second.erase(hdl);
    }
    Logger::info("WebSocket connection closed");
}

void WebSocketServer::onMessage(ConnectionHandle hdl, Server::message_ptr msg) {
    auto start = std::chrono::high_resolution_clock::now(); // Start time
    try {
        std::string payload = msg->get_payload();
        
        // Parse message for subscription requests
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::istringstream iss(payload);
        std::string errors;

        if (Json::parseFromStream(reader, iss, &root, &errors)) {
            if (root["type"].asString() == "subscribe") {
                handleSubscription(hdl, payload);
            }
        }

        // Forward message to handler if set
        if (messageHandler) {
            messageHandler(payload);
        }
    } catch (const std::exception& e) {
        Logger::error("Error processing WebSocket message: ", e.what());
    }
    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double, std::milli> latency = end - start;
    Logger::info("WebSocket message propagation delay: ", latency.count(), " ms");
}

void WebSocketServer::handleSubscription(ConnectionHandle hdl, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::istringstream iss(message);
    std::string errors;

    if (Json::parseFromStream(reader, iss, &root, &errors)) {
        if (root.isMember("instrument")) {
            std::string instrument = root["instrument"].asString();
            subscriptions[instrument].insert(hdl);
            Logger::info("Client subscribed to instrument: ", instrument);
        }
    }
}

void WebSocketServer::broadcastToSubscribers(const std::string& instrument, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (subscriptions.find(instrument) != subscriptions.end()) {
        for (const auto& hdl : subscriptions[instrument]) {
            try {
                server.send(hdl, message, websocketpp::frame::opcode::text);
            } catch (const std::exception& e) {
                Logger::error("Error broadcasting message: ", e.what());
            }
        }
    }
}

void WebSocketServer::run(uint16_t port) {
    try {
        server.set_reuse_addr(true);
        server.listen(port);
        server.start_accept();
        
        running = true;
        Logger::info("WebSocket server running on port ", port);
        
        server.run();
    } catch (const std::exception& e) {
        Logger::error("WebSocket server error: ", e.what());
        throw;
    }
}

void WebSocketServer::stop() {
    if (running) {
        server.stop_listening();
        running = false;
    }
}

void WebSocketServer::setMessageHandler(MessageHandler handler) {
    messageHandler = handler;
}

bool WebSocketServer::isClientSubscribed(ConnectionHandle hdl, const std::string& instrument) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = subscriptions.find(instrument);
    return it != subscriptions.end() && it->second.find(hdl) != it->second.end();
}

WebSocketServer::~WebSocketServer() {
    stop();
}