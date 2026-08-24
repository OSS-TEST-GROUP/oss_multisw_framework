#ifndef MIDDLEWARE_LOOGER_LOG_HPP
#define MIDDLEWARE_LOOGER_LOG_HPP

#include <cstdio>
#include <sstream>
#include <string>
#include <unistd.h>

#include "Logger.hpp"

// 본문을 do { ... } while (0) 로 감싼다.
// 그냥 { ... } 이면 `if (c) LOG_DEBUG(x); else y;` 에서 매크로 뒤의 세미콜론이
// if 문을 닫아버려 else 가 떠버린다. 지금 그렇게 쓴 곳은 없지만 언제든 터진다.
// 레벨 가드를 매크로 쪽에 둔다.
// 예전에는 Logger::log() 안에서야 레벨을 봐서, 버려질 DEBUG/VERBOSE 메시지도
// stringstream 생성 + getpid() 시스콜 + 전체 포맷팅 + 문자열 복사 2회를 전부
// 치른 뒤에 버려졌다. ContainerHealthMonitor 폴링 루프처럼 자주 도는 곳에서는
// 순수 낭비다.
// msg 를 괄호로 감쌀 수 없다 (bugprone-macro-parentheses).
// 이 매크로의 msg 는 완결된 식이 아니라 `"a" << x << "b"` 같은 스트림 삽입
// 조각이고, 아래에서 outStr 에 이어붙여야 비로소 의미가 된다. `(msg)` 로 감싸면
// `("a" << x)` 가 되어 스트림이 아닌 const char* 에 operator<< 를 적용하게 되고
// 컴파일이 깨진다. 인자를 문자열 하나로 제한하지 않는 한 고칠 수 없는 지적이다.
// 진단은 #define 줄이 아니라 매크로 본문의 `<< msg` 줄에 붙고, 그 줄은
// 백슬래시로 이어진 매크로 내부라 NOLINTNEXTLINE 을 끼워 넣을 수 없다
// (주석이 줄 이음을 삼킨다). 정의 전체를 NOLINTBEGIN/END 로 감싼다.
// NOLINTBEGIN(bugprone-macro-parentheses)
#define MMF_LOG_EMIT(level, level_fn, msg) do {                               \
  Logger* mmf_lg_ = Logger::Get();                                            \
  if (mmf_lg_ && mmf_lg_->ShouldLog(level)) {                                 \
    std::stringstream outStr;                                                 \
    outStr << __FUNCTION__ << "(" << __LINE__ << ") ["                        \
           << getpid() << "] : " << msg;                                      \
    mmf_lg_->level_fn(outStr.str());                                          \
  }                                                                           \
} while (0)
// NOLINTEND(bugprone-macro-parentheses)

#define LOG_INFO(msg)    MMF_LOG_EMIT(LogLevel::INFO,     info,     msg)
#define LOG_FATAL(msg)   MMF_LOG_EMIT(LogLevel::CRITICAL, critical, msg)
#define LOG_ERROR(msg)   MMF_LOG_EMIT(LogLevel::ERROR,    error,    msg)
#define LOG_DEBUG(msg)   MMF_LOG_EMIT(LogLevel::DEBUG,    debug,    msg)
#define LOG_VERBOSE(msg) MMF_LOG_EMIT(LogLevel::VERBOSE,  verbose,  msg)

#endif // #define MIDDLEWARE_LOOGER_LOG_HPP
