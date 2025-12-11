#ifndef MOCK_LOGGER_HPP_
#define MOCK_LOGGER_HPP_

#include <gmock/gmock.h>
#include "logger/Logger.hpp"

class MockLogger : public Logger {
public:
    MockLogger(LogLevel level = LogLevel::INFO, const std::string& filename = "") : Logger(level, filename) {}
    ~MockLogger(){}

    MOCK_METHOD(void, setLogLevel, (LogLevel level), ());
    MOCK_METHOD(void, critical, (const std::string msg), ());
    MOCK_METHOD(void, error, (const std::string msg), ());
    MOCK_METHOD(void, info, (const std::string msg), ());
    MOCK_METHOD(void, debug, (const std::string msg), ());
    MOCK_METHOD(void, verbose, (const std::string msg), ());

    MOCK_METHOD(Logger*, Get, (), ());
};

MockLogger* Mock_Logger;

Logger::Logger(LogLevel level, const std::string& filename) {}
Logger::~Logger() {}
void Logger::setLogLevel(LogLevel level) {
    Mock_Logger->setLogLevel(level);
}
void Logger::critical(const std::string msg) {
    Mock_Logger->critical(msg);
}
void Logger::error(const std::string msg) {
    Mock_Logger->error(msg);
}
void Logger::info(const std::string msg) {
    Mock_Logger->info(msg);
}
void Logger::debug(const std::string msg) {
    Mock_Logger->debug(msg);
}
void Logger::verbose(const std::string msg) {
    Mock_Logger->verbose(msg);
}

Logger* Logger::Get() {
    return Mock_Logger->Get();
}


#endif // MOCK_LOGGER_HPP_
