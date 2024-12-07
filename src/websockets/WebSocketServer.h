// websockets/WebSocketServer.h
#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <websocketpp/config/asio_tls.hpp>
#include <websocketpp/server.hpp>
#include <map>
#include <set>
#include <mutex>
#include <functional>

class WebSocketServer {
public:
    using Server = websocketpp::server<websocketpp::config::asio_tls>;
    using MessageHandler = std::function<void(const std::string&)>;
    using ConnectionHandle = websocketpp::connection_hdl;
    using ConnectionPtr = Server::connection_ptr;

    WebSocketServer();
    ~WebSocketServer();

    void run(uint16_t port);
    void stop();
    void setMessageHandler(MessageHandler handler);
    void broadcastToSubscribers(const std::string& instrument, const std::string& message);
    bool isClientSubscribed(ConnectionHandle hdl, const std::string& instrument);

private:
    Server server;
    std::map<std::string, std::set<ConnectionHandle, std::owner_less<ConnectionHandle>>> subscriptions;
    std::mutex mutex;
    MessageHandler messageHandler;
    bool running;

    void onOpen(ConnectionHandle hdl);
    void onClose(ConnectionHandle hdl);
    void onMessage(ConnectionHandle hdl, Server::message_ptr msg);
    void handleSubscription(ConnectionHandle hdl, const std::string& message);
    void setupTLS();
};

#endif