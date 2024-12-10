# Trading System: Technical Documentation
## 1. Introduction
This is a high-performance trading system built for interacting with the Deribit API. The system supports real-time market data streaming, order management, and risk validation. It is implemented in C++ with modular components handling different aspects such as authentication, order management, and data streaming.

The system also integrates a WebSocket server for real-time communication and a Command-Line Interface (CLI) for user interaction.

## 2. Architecture Overview
The application is structured around several key modules:

Authentication Module: Handles secure access to the Deribit API using OAuth tokens.
Order Management: Provides functionality to place, modify, and cancel orders.
Market Data Manager: Manages market data streaming and order book retrieval.
Execution Manager: Processes and executes queued orders.
Risk Manager: Validates orders based on predefined risk parameters.
WebSocket Server: Streams real-time data to subscribed clients.
Command-Line Interface: Allows users to interact with the system locally.


## 3. Module Breakdown

### 3.1 Authentication Module

```Files:
auth/AuthManager.h
auth/AuthManager.cpp
```
Purpose:
Handles the authentication flow with the Deribit API, using client credentials to obtain and manage OAuth tokens.

Key Classes and Methods:
```
AuthManager:
```

Constructor:
Initializes the authentication manager with a client ID, client secret, and API URL.
```cpp
AuthManager authManager(clientId, clientSecret);
bool authenticate():
```
Performs the client credentials grant to fetch an access token.
Sends an HTTP POST request to the /public/auth endpoint.
Parses the JSON response to extract access_token and refresh_token.
```cpp
bool refreshToken():
```
Refreshes the access token using the stored refresh_token.
Updates the tokens upon success.
```
std::string getAccessToken():
```
Thread-safe method to retrieve the current access token for API calls.

### Implementation Highlights:
The authenticate() method constructs a JSON payload and sends it to Deribit's public/auth endpoint:

```cpp
Json::Value requestBody;
requestBody["grant_type"] = "client_credentials";
requestBody["client_id"] = clientId;
requestBody["client_secret"] = clientSecret;

HttpClient client;
std::string response = client.post(apiUrl + "public/auth", payload, headers);
```
If the authentication fails, an error is logged, and the process terminates.

### 3.2 Order Management Module
Files:
```
orders/OrderManager.h
orders/OrderManager.cpp
```
Purpose:
Provides the core functionality for placing, modifying, and canceling orders. It integrates with the ExecutionManager to handle order execution and fills.

Key Classes and Methods:
OrderManager:

Constructor:
Takes an AuthManager reference for authenticated API requests.
bool placeOrder(Order& order):
Constructs a JSON payload based on the Order object.
Sends a request to private/buy or private/sell endpoints, depending on the order's side.
Handles responses to update the Order object.
bool cancelOrder(const std::string& orderId):
Cancels an order on Deribit using the private/cancel endpoint.
bool modifyOrder(const std::string& orderId, Order& newOrder):
Cancels the existing order and places a new one with updated parameters.
void onFill(const Order& order, double amount, double price):
Callback invoked when an order is filled.
Updates internal order tracking and positions.
Order:

Represents an individual order with fields like orderId, instrumentName, side, amount, price, and status.
Implementation Highlights:
The placeOrder method performs the following:

Validates the order through the RiskManager.
Constructs a JSON payload and sends it to the Deribit API:

```cpp
Copy code
Json::Value params;
params["instrument_name"] = order.instrumentName;
params["amount"] = order.amount;
params["price"] = order.price;
params["type"] = order.type;
```
Parses the API response and updates the order status.


### 3.3 Market Data Manager
Files:
```
market/MarketDataManager.h
market/MarketDataManager.cpp
```

Purpose:
Handles market data streaming and retrieval. It manages subscriptions to instruments and maintains a local order book for each instrument.

Key Classes and Methods:
```
MarketDataManager:
```

Constructor:
Takes an AuthManager reference for authenticated API requests.
```cpp
void startMarketDataStreaming(WebSocketServer& webSocketServer):
```
Begins streaming market data to clients via the WebSocket server.
Subscribes to Deribit channels (e.g., book.BTC-PERPETUAL.100ms).
```cpp
std::shared_ptr<OrderBook> getOrderBook(const std::string& instrument):
```
Fetches the order book for a specific instrument.
```cpp
void handleOrderBookUpdate(const std::string& instrument, const Json::Value& data):
```
Updates the local order book with new bid/ask data received from Deribit.

Implementation Highlights:
The market data streaming process involves:

Subscribing to order book channels on Deribit:
```cpp
Json::Value subscriptionMsg;
subscriptionMsg["jsonrpc"] = "2.0";
subscriptionMsg["id"] = 123;
subscriptionMsg["method"] = "public/subscribe";
subscriptionMsg["params"]["channels"].append("book.BTC-PERPETUAL.100ms");
webSocketServer.broadcastToSubscribers("", Json::writeString(writer, subscriptionMsg));
```

Handling updates via handleOrderBookUpdate:
```cpp
orderBook->updateBid(price, quantity);
orderBook->updateAsk(price, quantity);
```

### 3.4 Execution Manager
Files:
```
execution/ExecutionManager.h
execution/ExecutionManager.cpp
```

Purpose:
Manages the execution of queued orders, ensuring they meet the necessary conditions before being sent to the exchange.

Key Classes and Methods:
```
ExecutionManager:
void addOrder(Order& order):
```
Adds an order to the execution queue.
```
void executionLoop():
```
Processes orders from the queue.
Simulates or executes market orders immediately and checks conditions for limit orders.


### 3.5 Risk Management
Files:
```
risk/RiskManager.h
risk/RiskManager.cpp
```
Purpose:
The RiskManager ensures that all orders comply with pre-defined risk parameters before being executed. It acts as a safeguard against unintended or excessive trades by validating the size, leverage, and overall portfolio exposure.

Key Classes and Methods:
```
RiskManager:
```
```cpp
bool validateOrder(const Order& order):
```
Ensures the order complies with position limits, order size restrictions, and leverage/margin requirements.
Returns true if the order passes all checks, otherwise logs a rejection reason.
```cpp
void updatePosition(const std::string& instrument, double amount, double price):
```
Updates the internal position tracking system after an order is executed.
```cpp
double calculateRiskExposure(const Order& order):
```
Calculates the overall risk exposure based on the order and the current portfolio.


Implementation Highlights:
The validateOrder function evaluates:

Order Size:
```cpp
if (order.amount > maxOrderSize) {
    Logger::warn("Order size exceeds the maximum allowed limit");
    return false;
}
```
Leverage:
```cpp
double marginRequired = calculateMargin(order);
if (marginRequired > availableMargin) {
    Logger::warn("Insufficient margin for the order");
    return false;
}
```
Position Limits: Checks that the total position size for an instrument does not exceed predefined limits.


### 3.6 WebSocket Server
Files:
```
websocket/WebSocketServer.h
websocket/WebSocketServer.cpp
```
Purpose:
The WebSocket server streams real-time market data and updates to connected clients. It also manages subscriptions for specific instruments, ensuring clients only receive relevant data.

Key Classes and Methods:
```
WebSocketServer:
```
```cpp
void run(uint16_t port):
```
Initializes and starts the WebSocket server on the specified port (e.g., 9002).
```cpp
void broadcastToSubscribers(const std::string& instrument, const std::string& message):
```
Sends a message to all clients subscribed to a particular instrument.
```cpp
void onClientMessage(const std::string& clientId, const std::string& message):
```
Handles incoming messages from clients, such as subscription requests.
```cpp
void addSubscription(const std::string& clientId, const std::string& instrument):
```
Adds a client to the subscription list for a specific instrument.

Implementation Highlights:
The WebSocket server uses websocketpp to handle connections and manage data broadcasting.
Subscription management ensures efficient message routing:
```cpp
clientSubscriptions[clientId].insert(instrument);
```

Example of broadcasting an order book update:

```cpp
void WebSocketServer::broadcastToSubscribers(const std::string& instrument, const std::string& message) {
    for (const auto& client : subscribers[instrument]) {
        wsConnection->send(client, message, websocketpp::frame::opcode::text);
    }
}
```

### 3.7 Command-Line Interface (CLI)
Files:
```
main.cpp
```
Purpose:
The CLI provides a menu-driven interface for interacting with the system. It allows local users to place orders, fetch market data, modify existing orders, and view current positions.

Features:
1. Place an Order
2. Cancel an Order
3. Modify an Order
4. View Order Book
5. View Current Positions
6. Exit the System

Implementation Highlights:
The CLI is implemented as an interactive loop in main.cpp. It reads user input, invokes corresponding methods, and displays results in the terminal.

Example menu display:
```cpp
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
```

Example placeOrder logic:
```cpp
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
```


## 4. Logging
The system uses a centralized logging utility (Logger) to track events, errors, and debug information.

Logging Levels:
INFO: General events (e.g., initialization, successful actions).
WARN: Warnings about potential issues (e.g., large orders, low margin).
ERROR: Errors during execution (e.g., API failures, invalid parameters).
FATAL: Critical errors causing system shutdown.
Example:

```cpp
Logger::info("Starting WebSocket server on port 9002");
Logger::error("Market data streaming error: ", e.what());
```

## 5. Workflow
### Initialization
Authenticate with the Deribit API using AuthManager.
Start the WebSocket server for real-time data streaming.
Initialize the MarketDataManager for market data retrieval.
### Order Placement
The CLI or an automated strategy triggers OrderManager::placeOrder.
The RiskManager validates the order against position limits and leverage requirements.
If valid, the order is sent to Deribit via API.
### Market Data Streaming
Subscribe to Deribit order book channels (e.g., book.BTC-PERPETUAL.100ms).
Stream real-time updates to connected WebSocket clients.

## 6. Configuration
### Dependencies:

Install the following libraries:
curl: For making HTTP requests.
jsoncpp: For JSON handling.
boost: For threading, asynchronous tasks, and WebSocket support.
websocketpp: For implementing the WebSocket server.
Use a package manager like apt-get (Linux) or brew (macOS) to install them:
```bash
sudo apt-get install libcurl4-openssl-dev libjsoncpp-dev libboost-all-dev
```
TLS Certificates (For WebSocket):

Place the required SSL certificates (cert.pem and key.pem) in the project directory if secure WebSocket connections are required.
Configuration Parameters:

Update clientId and clientSecret in main.cpp with valid Deribit API credentials.
Ensure Deribit’s test environment or live environment is selected correctly in AuthManager.
Build and Run

Compilation: Compile the project with the following command:

```bash
g++ -std=c++17 -pthread -lssl -lcrypto -lcurl -o GoQuant \
    main.cpp \
    auth/AuthManager.cpp auth/utils/HttpClient.cpp \
    orders/OrderManager.cpp market/MarketDataManager.cpp \
    websocket/WebSocketServer.cpp src/Logger.cpp
```
Execution: Run the compiled binary:
```bash
./GoQuant
```
Using the CLI: Interact with the application through the terminal menu. For example:
Option 1: Place a buy or sell order.
Option 2: Cancel an active order.
Option 4: View the order book for a specific instrument.



## 7. Future Improvements
REST API Interface:
Expose RESTful endpoints for external systems to interact with GoQuant (e.g., place orders, fetch positions).

Enhanced Risk Management:
Add configurable risk profiles for different trading strategies.

WebSocket Enhancements:
Allow WebSocket clients to send commands (e.g., placing orders or subscribing to specific updates).

Automated Strategies:
Integrate trading algorithms that operate based on real-time market conditions and historical data analysis.

Logging Enhancements:
Add support for structured logs (e.g., JSON format) for easier parsing and integration with log analysis tools.

Test Suite:
Expand automated testing with mock API responses and stress tests for WebSocket connections.

UI Development:
Build a graphical user interface for easier interaction, potentially using Electron or Qt.
