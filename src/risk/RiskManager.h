// risk/RiskManager.h
#ifndef RISK_MANAGER_H
#define RISK_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "../orders/models/Order.h"

class RiskManager {
public:
    struct RiskLimits {
        double maxOrderSize;
        double maxPositionSize;
        double maxLeverage;
        double minMargin;
        double maxDailyLoss;
        int maxOrdersPerSecond;
    };

    RiskManager();
    bool validateOrder(const Order& order);
    void updatePosition(const std::string& instrument, double amount, double price);
    void setRiskLimits(const RiskLimits& limits);

private:
    RiskLimits limits;
    std::unordered_map<std::string, double> positions;  // instrument -> position size
    std::unordered_map<std::string, double> avgPrices;  // instrument -> average price
    std::mutex riskMutex;

    // Risk check methods
    bool checkOrderSize(const Order& order);
    bool checkPositionLimit(const Order& order);
    bool checkLeverage(const Order& order);
    bool checkMargin(const Order& order);
    bool checkRateLimit(const std::string& instrument);
};

#endif