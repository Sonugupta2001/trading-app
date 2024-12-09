#include "RiskManager.h"
#include "../include/Logger.h"

RiskManager::RiskManager() {
    limits.maxOrderSize = 1.0;
    limits.maxPositionSize = 5.0;
    limits.maxLeverage = 10.0;
    limits.minMargin = 0.1;
    limits.maxDailyLoss = 1000.0;
    limits.maxOrdersPerSecond = 5;
}

bool RiskManager::validateOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(riskMutex);
    
    try {
        if (!checkOrderSize(order)) {
            Logger::warning("Order rejected: Exceeds maximum order size");
            return false;
        }

        if (!checkPositionLimit(order)) {
            Logger::warning("Order rejected: Would exceed position limit");
            return false;
        }

        if (!checkLeverage(order)) {
            Logger::warning("Order rejected: Would exceed leverage limit");
            return false;
        }

        if (!checkMargin(order)) {
            Logger::warning("Order rejected: Insufficient margin");
            return false;
        }

        if (!checkRateLimit(order.instrumentName)) {
            Logger::warning("Order rejected: Rate limit exceeded");
            return false;
        }

        Logger::info("Order passed all risk checks");
        return true;

    } catch (const std::exception& e) {
        Logger::error("Error in risk validation: ", e.what());
        return false;
    }
}

bool RiskManager::checkOrderSize(const Order& order) {
    return order.amount <= limits.maxOrderSize;
}

bool RiskManager::checkPositionLimit(const Order& order) {
    double currentPosition = positions[order.instrumentName];
    double newPosition = currentPosition;
    
    if (order.side == "buy") {
        newPosition += order.amount;
    } else {
        newPosition -= order.amount;
    }
    
    return std::abs(newPosition) <= limits.maxPositionSize;
}

bool RiskManager::checkLeverage(const Order& order) {
    // Simplified leverage calculation
    double totalPosition = order.amount * order.price;
    double currentMargin = totalPosition / limits.maxLeverage;
    return currentMargin >= limits.minMargin;
}

bool RiskManager::checkMargin(const Order& order) {
    // Implement margin calculation based on your specific requirements
    return true; // Placeholder
}

bool RiskManager::checkRateLimit(const std::string& instrument) {
    // Implement rate limiting logic
    return true; // Placeholder
}

void RiskManager::updatePosition(const std::string& instrument, double amount, double price) {
    std::lock_guard<std::mutex> lock(riskMutex);
    
    positions[instrument] += amount;
    // Update average price calculation
    double oldPosition = positions[instrument] - amount;
    double oldAvgPrice = avgPrices[instrument];
    
    if (std::abs(positions[instrument]) > 0) {
        avgPrices[instrument] = (oldPosition * oldAvgPrice + amount * price) / positions[instrument];
    } else {
        avgPrices[instrument] = 0;
    }
}