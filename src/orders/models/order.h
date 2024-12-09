#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <chrono>

struct Order {
    std::string orderId;
    std::string instrumentName;
    std::string side;
    double amount;
    double price;
    std::string type;
    std::string status;
    double filledAmount;
    double averageFilledPrice;
    std::chrono::system_clock::time_point timestamp;
    std::string rejectionReason;
};

#endif