#ifndef ADSW_AGL_LOGGER_HPP_
#define ADSW_AGL_LOGGER_HPP_

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
    WARN,
    INFO,
    DEBUG
};

class Logger {
public:
    Logger(LogLevel level = LogLevel::INFO, const std::string& filename = "");
    ~Logger();

    void setLogLevel(LogLevel level);

    void critical(const std::string& msg);
    void error(const std::string& msg);
    void warn(const std::string& msg);
    void info(const std::string& msg);
    void debug(const std::string& msg);

    static Logger* Get();

private:
    static std::shared_ptr<Logger> getInstance();

    LogLevel logLevel_;
    std::ofstream fileStream_;
    bool useFile_ = false;
    std::mutex logMutex_;

    void log(LogLevel level, const std::string& levelStr, const std::string& msg);
    std::string timestamp();
};

#endif // ADSW_AGL_LOGGER_HPP_
