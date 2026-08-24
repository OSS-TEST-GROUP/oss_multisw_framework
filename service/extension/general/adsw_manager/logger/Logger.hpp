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

    void critical(const std::string& msg);
    void error(const std::string& msg);
    void info(const std::string& msg);
    void debug(const std::string& msg);
    void verbose(const std::string& msg);

    // 이 레벨이 실제로 기록되는지. LOG_* 매크로가 메시지를 조립하기 *전에*
    // 물어본다. 예전에는 log() 안에서야 레벨을 봐서, 버려질 메시지도
    // stringstream 생성 + getpid() + 포맷팅 + 문자열 복사 2회를 다 치렀다.
    bool ShouldLog(LogLevel level) const;

    static Logger* Get();

private:
    static std::shared_ptr<Logger> getInstance();

private:
    LogLevel logLevel;
    std::ofstream fileStream;
    bool useFile = false;
    std::mutex logMutex;

    void log(LogLevel level, const std::string& levelStr, const std::string& msg);

    std::string timestamp();
};

#endif  // MIDDLEWARE_LOOGER_LOGGER_HPP