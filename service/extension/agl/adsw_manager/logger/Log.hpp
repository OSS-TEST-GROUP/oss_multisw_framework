#ifndef ADSW_AGL_LOG_HPP_
#define ADSW_AGL_LOG_HPP_

#include <stdio.h>
#include <sstream>
#include <unistd.h>

#include "Logger.hpp"

#define LOG_DEBUG(msg) do { \
    std::stringstream _ss; \
    _ss << __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg; \
    Logger::Get()->debug(_ss.str()); \
} while(0)

#define LOG_INFO(msg) do { \
    std::stringstream _ss; \
    _ss << __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg; \
    Logger::Get()->info(_ss.str()); \
} while(0)

#define LOG_WARN(msg) do { \
    std::stringstream _ss; \
    _ss << __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg; \
    Logger::Get()->warn(_ss.str()); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::stringstream _ss; \
    _ss << __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg; \
    Logger::Get()->error(_ss.str()); \
} while(0)

#endif // ADSW_AGL_LOG_HPP_
