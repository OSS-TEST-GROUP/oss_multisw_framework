#ifndef LOGGER_H
#define LOGGER_H

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#ifdef BUILD_FOR_ANDROID
#include <android/log.h>
#define LOG_TAG "VISS_APP"
#endif

enum class LogPriority {
      UNKNOWN = 0,   // ANDROID_LOG_UNKNOWN = 0,
      DEFAULT,       // ANDROID_LOG_DEFAULT,
      VERBOSE,       // ANDROID_LOG_VERBOSE,
      DEBUG,         // ANDROID_LOG_DEBUG,
      INFO,          // ANDROID_LOG_INFO,
      WARN,          // ANDROID_LOG_WARN,
      ERROR,         // ANDROID_LOG_ERROR,
      FATAL,         // ANDROID_LOG_FATAL,
      SILENT         // ANDROID_LOG_SILENT
};

/// 출력 하한선. 이 값보다 낮은 우선순위는 **아무것도 쓰지 않는다**.
///
/// 왜 필요한가: 예전에는 레벨 게이트가 없어 `LOG_DEBUG` 도 무조건 stdout 으로 나갔다.
/// VISS 구독이 붙어 있는 동안 응답 JSON 과 ObjectPool 메시지가 샘플마다 쌓여
/// 컨테이너 로그 하나가 **376GB** 까지 자랐고(2026-09-03, 151 보드 실측) 디스크가
/// 100% 차서 헬스체크 exec 조차 실패했다. 도커 로테이션은 상한만 정할 뿐 양을 줄이지
/// 못하므로, 발생원에서 끄는 수단이 필요하다.
///
/// 제어: 환경변수 `MMF_LOG_LEVEL` (error|warn|info|debug|verbose). 기본 **info**.
/// 프로세스 최초 로그 시점에 한 번만 읽는다(정적 초기화, 스레드 안전).
inline LogPriority MmfLogThreshold() {
    static const LogPriority kThreshold = [] {
        const char* e = std::getenv("MMF_LOG_LEVEL");
        if (e == nullptr) {
            return LogPriority::INFO;
        }
        std::string v(e);
        std::transform(v.begin(), v.end(), v.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (v == "verbose") return LogPriority::VERBOSE;
        if (v == "debug")   return LogPriority::DEBUG;
        if (v == "info")    return LogPriority::INFO;
        if (v == "warn" || v == "warning") return LogPriority::WARN;
        if (v == "error")   return LogPriority::ERROR;
        if (v == "silent")  return LogPriority::SILENT;
        return LogPriority::INFO;   // 알 수 없는 값은 기본값으로
    }();
    return kThreshold;
}

class LogStream : public std::ostringstream {
public:
    LogStream(LogPriority priority) : priority_(priority) {}

    ~LogStream() {
        if (priority_ < MmfLogThreshold()) {
            return;   // 하한선 미달 → 출력하지 않는다
        }
#ifdef BUILD_FOR_ANDROID
        __android_log_write(static_cast<int>(priority_), LOG_TAG, str().c_str());
#else
        std::ostream& stream = (priority_ >= LogPriority::WARN) ? std::cerr : std::cout;
        stream << str() << std::endl;
#endif
    }

private:
    LogPriority priority_;
};

#define LOG_INFO LogStream(LogPriority::INFO)
#define LOG_ERROR LogStream(LogPriority::ERROR)
#define LOG_WARN LogStream(LogPriority::WARN)
#define LOG_DEBUG LogStream(LogPriority::DEBUG)
#endif // LOGGER_H
