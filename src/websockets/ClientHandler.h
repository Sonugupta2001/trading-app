#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <string>

class ClientHandler {
public:
    ClientHandler();
    ~ClientHandler();

    void onMessage(websocketpp::connection_hdl hdl, const std::string& message);
    void subscribeToMarketData(const std::string& symbol);
    void sendMarketData(websocketpp::connection_hdl hdl, const std::string& data);

private:
    // Add any other necessary attributes to manage client states
};

#endif // CLIENTHANDLER_H