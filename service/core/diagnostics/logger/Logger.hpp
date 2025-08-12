#ifndef MIDDLEWARE_LOOGER_LOGGER_HPP
#define MIDDLEWARE_LOOGER_LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <sstream>
#include <memory>

enum class LogLevel {
    CRITICAL = 0,
    ERROR,
    INFO,
    DEBUG,
    VERBOSE
};


class Logger {
public:
    Logger(LogLevel level = LogLevel::INFO, const std::string& filename = "");
    ~Logger();

    void setLogLevel(LogLevel level);

    void critical(const std::string msg); 
    void error(const std::string msg); 
    void info(const std::string msg);
    void debug(const std::string msg);
    void verbose(const std::string msg);

    static Logger* Get();

private:
    static std::shared_ptr<Logger> getInstance();

private:
    LogLevel logLevel;
    std::ofstream fileStream;
    bool useFile = false;
    std::mutex logMutex;

    void log(LogLevel level, const std::string& levelStr, const std::string msg);

    std::string timestamp();
};

#endif  // MIDDLEWARE_LOOGER_LOGGER_HPP