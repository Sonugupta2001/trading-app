#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <chrono>

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    static void init(const std::string& logFile = "trading_system.log");
    static void setLogLevel(Level level);
    
    template<typename... Args>
    static void debug(const Args&... args) {
        log(DEBUG, args...);
    }
    
    template<typename... Args>
    static void info(const Args&... args) {
        log(INFO, args...);
    }
    
    template<typename... Args>
    static void warning(const Args&... args) {
        log(WARNING, args...);
    }
    
    template<typename... Args>
    static void error(const Args&... args) {
        log(ERROR, args...);
    }
    
    template<typename... Args>
    static void fatal(const Args&... args) {
        log(FATAL, args...);
    }

private:
    static std::ofstream logFile;
    static std::mutex logMutex;
    static Level logLevel;
    
    template<typename... Args>
    static void log(Level level, const Args&... args) {
        if (level >= logLevel) {
            std::stringstream ss;
            (ss << ... << args);
            writeLog(level, ss.str());
        }
    }
    
    static void writeLog(Level level, const std::string& message);
    static std::string getCurrentTimestamp();
    static std::string getLevelString(Level level);
};

#endif