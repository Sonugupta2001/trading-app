// market/models/orderBook.h
#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <map>
#include <mutex>
#include <string>

class OrderBook {
public:
    OrderBook(const std::string& instrument);
    
    // Update methods
    void updateBid(double price, double quantity);
    void updateAsk(double price, double quantity);
    void clear();

    // Getter methods
    double getBestBid() const;
    double getBestAsk() const;
    double getMidPrice() const;
    double getSpread() const;

    // Get full order book depth
    const std::map<double, double>& getBids() const;
    const std::map<double, double>& getAsks() const;

private:
    std::string instrument;
    std::map<double, double> bids;  // price -> quantity
    std::map<double, double> asks;  // price -> quantity
    mutable std::mutex bookMutex;
};

#endif