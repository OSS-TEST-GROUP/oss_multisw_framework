#ifndef GENERAL_ADSW_MANAGER_UNITTEST_MOCK_LOGGER_HPP_
#define GENERAL_ADSW_MANAGER_UNITTEST_MOCK_LOGGER_HPP_

#include <gmock/gmock.h>
#include "logger/Logger.hpp"

// Logger 링크 심.
//
// 이 저장소의 mock 은 gmock 서브클래스가 아니라 실제 클래스의 메서드를 인라인
// 정의해 링커가 그쪽을 집게 하는 방식이다. 그래서 테스트 바이너리는 실제
// Logger.cpp 를 링크하지 않는다.
class MockLogger : public Logger {
public:
    MockLogger(LogLevel level = LogLevel::INFO, const std::string& filename = "")
        : Logger(level, filename) {}
    ~MockLogger() {}

    MOCK_METHOD(void, setLogLevel, (LogLevel level), ());
    MOCK_METHOD(void, critical, (const std::string& msg), ());
    MOCK_METHOD(void, error,    (const std::string& msg), ());
    MOCK_METHOD(void, info,     (const std::string& msg), ());
    MOCK_METHOD(void, debug,    (const std::string& msg), ());
    MOCK_METHOD(void, verbose,  (const std::string& msg), ());

    MOCK_METHOD(Logger*, Get, (), ());
    MOCK_METHOD(bool, ShouldLog, (LogLevel level), (const));
};

MockLogger* Mock_Logger = nullptr;

Logger::Logger(LogLevel, const std::string&) {}
Logger::~Logger() {}
void Logger::setLogLevel(LogLevel level) { Mock_Logger->setLogLevel(level); }
void Logger::critical(const std::string& msg) { Mock_Logger->critical(msg); }
void Logger::error(const std::string& msg)    { Mock_Logger->error(msg); }
void Logger::info(const std::string& msg)     { Mock_Logger->info(msg); }
void Logger::debug(const std::string& msg)    { Mock_Logger->debug(msg); }
void Logger::verbose(const std::string& msg)  { Mock_Logger->verbose(msg); }

// 정의를 inline 으로 두면 이 헤더를 include 하지 않는 TU(ContainerController.cpp 등)
// 에서 심볼을 못 찾는다. 링크 심은 반드시 외부 링키지여야 하고, 그래서 이 헤더는
// 테스트 바이너리당 정확히 한 TU 에서만 include 해야 한다 (ros2 판과 같은 규약).
//
// 테스트에서는 항상 기록한다고 본다. 그래야 LOG_* 본문이 실행되어
// 기존 EXPECT_CALL(debug/info) 이 유효하고 커버리지도 프로덕션과 같게 잡힌다.
bool Logger::ShouldLog(LogLevel) const { return true; }

Logger* Logger::Get() { return Mock_Logger; }

// 모든 테스트가 반복하는 준비/정리. NiceMock 이라 관심 없는 호출은 조용하다.
class ScopedMockLogger {
public:
    ScopedMockLogger() {
        Mock_Logger = new ::testing::NiceMock<MockLogger>();
    }
    ~ScopedMockLogger() {
        delete Mock_Logger;
        Mock_Logger = nullptr;
    }
};

#endif // GENERAL_ADSW_MANAGER_UNITTEST_MOCK_LOGGER_HPP_
