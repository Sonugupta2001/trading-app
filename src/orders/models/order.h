// orders/models/Order.h
#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <chrono>

struct Order {
    std::string orderId;
    std::string instrumentName;
    std::string side;           // "buy" or "sell"
    double amount;
    double price;
    std::string type;          // "limit" or "market"
    std::string status;        // "pending", "filled", "cancelled", "rejected"
    double filledAmount;
    double averageFilledPrice;
    std::chrono::system_clock::time_point timestamp;
    std::string rejectionReason;
};

#endif