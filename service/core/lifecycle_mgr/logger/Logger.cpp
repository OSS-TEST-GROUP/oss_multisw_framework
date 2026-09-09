#include "Logger.hpp"



Logger::Logger(LogLevel level, const std::string& filename)
        : logLevel(level)
{
    if (!filename.empty()) {
        fileStream.open(filename, std::ios::app);
        useFile = fileStream.is_open();
    }
}

Logger::~Logger() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

Logger* Logger::Get(){
   return getInstance().get();
}

std::shared_ptr<Logger> Logger::getInstance()
{
    // Note we need a custom deleter, since the destructor is protected.
    // 기본 레벨을 DEBUG 로 박아 두면 정상 동작의 부산물이 그대로 쌓인다 —
    // 151 보드에서 컨테이너 로그 하나가 376GB 까지 자란 사고의 같은 계열 원인이다
    // (2026-09-03). MMF_LOG_LEVEL 로 제어하고 기본은 INFO 로 둔다.
    static std::shared_ptr<Logger> instance(
        new Logger(MmfLogLevelFromEnv(), "lifecycle_mgr.log"),
        [](Logger* p)
        {
            delete p;
        });
    return instance;
}

void Logger::setLogLevel(LogLevel level) { logLevel = level; }

bool Logger::ShouldLog(LogLevel level) const { return level <= logLevel; }

void Logger::critical(const std::string& msg) { 
    log(LogLevel::CRITICAL, "[CRITICAL]", msg); 
}
void Logger::error(const std::string& msg) { 
    log(LogLevel::ERROR, "[ERROR]", msg); 
}
void Logger::info(const std::string& msg)    
{ 
    log(LogLevel::INFO,    "[INFO]",    msg); 
}
void Logger::debug(const std::string& msg)   
{ 
    log(LogLevel::DEBUG,   "[DEBUG]",   msg); 
}
void Logger::verbose(const std::string& msg) 
{ 
    log(LogLevel::VERBOSE, "[VERBOSE]", msg); 
}


void Logger::log(LogLevel level, const std::string& levelStr, const std::string& msg) {
    if (level > logLevel) {
        return;
    }

    std::lock_guard<std::mutex> guard(logMutex);

    std::ostringstream oss;

    oss << "[" << timestamp() << "] [" << levelStr << "] " << msg << std::endl;

    if (useFile) {
        fileStream << oss.str();
        fileStream.flush();
    } else {
        std::cout << oss.str();
    }
}


std::string Logger::timestamp() {
    std::time_t now = std::time(nullptr);
    
    char buf[20];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
        return buf;
    }
    return "";
}