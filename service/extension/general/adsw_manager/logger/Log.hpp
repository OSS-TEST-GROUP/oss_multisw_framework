#ifndef MIDDLEWARE_LOOGER_LOG_HPP
#define MIDDLEWARE_LOOGER_LOG_HPP

#include <stdio.h>
#include <string>
#include <sstream>
#include <unistd.h>

#include "Logger.hpp"

#define LOG_INFO(msg) {            \
  std::stringstream outStr;            \
  outStr<< __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg;    \
  Logger::Get()->info(outStr.str());    \
}


#define LOG_FATAL(msg) {            \
  std::stringstream outStr;            \
  outStr<< __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg;    \
  Logger::Get()->critical(outStr.str());    \
}

#define LOG_ERROR(msg) {            \
  std::stringstream outStr;            \
  outStr<< __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg;    \
  Logger::Get()->error(outStr.str());    \
}

#define LOG_DEBUG(msg) {            \
  std::stringstream outStr;            \
  outStr<< __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg;    \
  Logger::Get()->debug(outStr.str());    \
}

#define LOG_VERBOSE(msg) {            \
  std::stringstream outStr;            \
  outStr<< __FUNCTION__ << "(" << __LINE__ << ") [" << getpid() << "] : " << msg;    \
  Logger::Get()->verbose(outStr.str());    \
}

#endif // #define MIDDLEWARE_LOOGER_LOG_HPP

