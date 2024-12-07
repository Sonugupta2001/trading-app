// src/Logger.cpp
#include "Logger.h"
#include <iostream>
#include <iomanip>

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;
Logger::Level Logger::logLevel = Logger::INFO;

void Logger::init(const std::string& filename) {
    logFile.open(filename, std::ios::app);
    if (!logFile.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

void Logger::setLogLevel(Level level) {
    logLevel = level;
}

void Logger::writeLog(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string output = getCurrentTimestamp() + " [" + getLevelString(level) + "] " + message + "\n";
    
    logFile << output;
    logFile.flush();
    
    // Also print to console for ERROR and FATAL
    if (level >= ERROR) {
        std::cerr << output;
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::getLevelString(Level level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}