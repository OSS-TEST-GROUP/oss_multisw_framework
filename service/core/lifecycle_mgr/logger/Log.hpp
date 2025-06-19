#ifndef MIDDLEWARE_LOOGER_LOG_HPP
#define MIDDLEWARE_LOOGER_LOG_HPP

#include "Logger.hpp"

#define LOG_INFO(message) Logger::Get()->info(message)
#define LOG_FATAL(message) Logger::Get()->critical(message)
#define LOG_DEBUG(message) Logger::Get()->debug(message)
#define LOG_VERBOSE(message) Logger::Get()->verbose(message)

#endif // LIFECYCLE_LIFECYCLE_FACTORY_HPP_
