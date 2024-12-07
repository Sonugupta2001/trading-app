#include "orderBook.h"
#include <stdexcept>

OrderBook::OrderBook(const std::string& instrument) : instrument(instrument) {}

void OrderBook::updateBid(double price, double quantity) {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (quantity <= 0) {
        bids.erase(price);
    } else {
        bids[price] = quantity;
    }
}

void OrderBook::updateAsk(double price, double quantity) {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (quantity <= 0) {
        asks.erase(price);
    } else {
        asks[price] = quantity;
    }
}

void OrderBook::clear() {
    std::lock_guard<std::mutex> lock(bookMutex);
    bids.clear();
    asks.clear();
}

double OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (bids.empty()) {
        return 0.0;
    }
    return bids.rbegin()->first;
}

double OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (asks.empty()) {
        return 0.0;
    }
    return asks.begin()->first;
}

double OrderBook::getMidPrice() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (bids.empty() || asks.empty()) {
        return 0.0;
    }
    return (getBestBid() + getBestAsk()) / 2.0;
}

double OrderBook::getSpread() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    if (bids.empty() || asks.empty()) {
        return 0.0;
    }
    return getBestAsk() - getBestBid();
}

const std::map<double, double>& OrderBook::getBids() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    return bids;
}

const std::map<double, double>& OrderBook::getAsks() const {
    std::lock_guard<std::mutex> lock(bookMutex);
    return asks;
}