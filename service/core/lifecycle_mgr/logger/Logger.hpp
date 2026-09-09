#ifndef MIDDLEWARE_LOOGER_LOGGER_HPP
#define MIDDLEWARE_LOOGER_LOGGER_HPP

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
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

/// 기본 로그 레벨을 환경변수 `MMF_LOG_LEVEL` 에서 읽는다 (error|warn|info|debug|verbose).
/// 미설정·미인식이면 **INFO**. 예전엔 getInstance() 가 DEBUG 를 박아 두어 정상 동작의
/// 부산물이 계속 쌓였다 — 컨테이너 로그가 376GB 까지 자란 사고(2026-09-03)의 같은 계열
/// 원인이라 발생원에서 끌 수 있게 한다. vehicle_info_mgr 의 Logger.h 와 같은 변수를 쓴다.
inline LogLevel MmfLogLevelFromEnv() {
    const char* e = std::getenv("MMF_LOG_LEVEL");
    if (e == nullptr) {
        return LogLevel::INFO;
    }
    std::string v(e);
    std::transform(v.begin(), v.end(), v.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "verbose") return LogLevel::VERBOSE;
    if (v == "debug")   return LogLevel::DEBUG;
    if (v == "info")    return LogLevel::INFO;
    // 이 로거의 레벨에는 WARN 이 없다(CRITICAL/ERROR/INFO/DEBUG/VERBOSE) —
    // warn 을 주면 한 단계 엄격한 ERROR 로 해석한다.
    if (v == "warn" || v == "warning" || v == "error") return LogLevel::ERROR;
    return LogLevel::INFO;   // 알 수 없는 값은 기본값으로
}



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