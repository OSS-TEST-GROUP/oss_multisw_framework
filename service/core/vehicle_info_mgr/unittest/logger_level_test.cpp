#include <gtest/gtest.h>

#include <cstdlib>
#include <sstream>

#include "Logger.h"

// 로그 레벨 게이트 — 발생원에서 로그 양을 줄이는 수단이다.
//
// 왜 이 테스트가 있나: 예전에는 게이트가 없어 LOG_DEBUG 도 무조건 stdout 으로 나갔고,
// VISS 구독이 붙어 있는 동안 응답 JSON 이 샘플마다 쌓여 컨테이너 로그 하나가 376GB 까지
// 자랐다(2026-09-03, 151 보드). 도커 로테이션은 상한만 정하므로 발생원 차단이 필요하다.
//
// MmfLogThreshold() 는 정적 초기화라 프로세스당 한 번만 읽는다 → 한 테스트 프로세스에서
// 여러 값을 시험할 수 없다. 그래서 여기서는 **기본값 동작**과 **파싱 규칙**을 검증하고,
// 값별 억제는 아래 헬퍼로 순수 함수처럼 확인한다.
namespace {

// 실제 게이트와 같은 비교 규칙 (LogStream::~LogStream 참조).
bool Emits(LogPriority msg, LogPriority threshold) { return !(msg < threshold); }

}  // namespace

TEST(LoggerLevel, 기본값은_info_이고_debug_는_억제된다) {
    // 테스트 실행 환경에 MMF_LOG_LEVEL 이 없어야 기본값 검증이 성립한다.
    ASSERT_EQ(std::getenv("MMF_LOG_LEVEL"), nullptr)
        << "MMF_LOG_LEVEL 이 설정된 환경에서는 이 케이스를 건너뛰어야 한다";
    EXPECT_EQ(MmfLogThreshold(), LogPriority::INFO);
}

TEST(LoggerLevel, 하한선_비교_규칙) {
    // info 하한선: debug/verbose 는 버리고 info 이상은 남긴다.
    EXPECT_FALSE(Emits(LogPriority::DEBUG,   LogPriority::INFO));
    EXPECT_FALSE(Emits(LogPriority::VERBOSE, LogPriority::INFO));
    EXPECT_TRUE (Emits(LogPriority::INFO,    LogPriority::INFO));
    EXPECT_TRUE (Emits(LogPriority::WARN,    LogPriority::INFO));
    EXPECT_TRUE (Emits(LogPriority::ERROR,   LogPriority::INFO));

    // debug 하한선: 전부 남긴다.
    EXPECT_TRUE(Emits(LogPriority::DEBUG, LogPriority::DEBUG));

    // error 하한선: info 까지 버린다(사고 시 최소 로그로 운용).
    EXPECT_FALSE(Emits(LogPriority::INFO,  LogPriority::ERROR));
    EXPECT_TRUE (Emits(LogPriority::ERROR, LogPriority::ERROR));
}

TEST(LoggerLevel, 하한선_미달_메시지는_스트림에_쓰이지_않는다) {
    ASSERT_EQ(std::getenv("MMF_LOG_LEVEL"), nullptr);

    std::ostringstream captured;
    std::streambuf* saved = std::cout.rdbuf(captured.rdbuf());
    { LOG_DEBUG << "이 줄은 기본 설정에서 나오지 않아야 한다"; }
    { LOG_INFO  << "이 줄은 나와야 한다"; }
    std::cout.rdbuf(saved);

    EXPECT_EQ(captured.str().find("나오지 않아야"), std::string::npos);
    EXPECT_NE(captured.str().find("나와야 한다"), std::string::npos);
}
