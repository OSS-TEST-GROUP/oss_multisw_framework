#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include <iostream>

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

class LogStream : public std::ostringstream {
public:
    LogStream(LogPriority priority) : priority_(priority) {}

    ~LogStream() {
#ifdef BUILD_FOR_ANDROID
        __android_log_write(static_cast<int>(priority_), LOG_TAG, str().c_str());
#else
        std::ostream& stream = (priority_ == LogPriority::INFO) ? std::cout : std::cerr;
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
