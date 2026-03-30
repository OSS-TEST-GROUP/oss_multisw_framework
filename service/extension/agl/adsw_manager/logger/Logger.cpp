#include "Logger.hpp"

Logger::Logger(LogLevel level, const std::string& filename)
    : logLevel_(level)
{
    if (!filename.empty()) {
        fileStream_.open(filename, std::ios::app);
        useFile_ = fileStream_.is_open();
    }
}

Logger::~Logger() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

Logger* Logger::Get() {
    return getInstance().get();
}

std::shared_ptr<Logger> Logger::getInstance() {
    static std::shared_ptr<Logger> instance(
        new Logger(LogLevel::DEBUG, "adsw_agl.log"),
        [](Logger* p) { delete p; });
    return instance;
}

void Logger::setLogLevel(LogLevel level) { logLevel_ = level; }

void Logger::critical(const std::string& msg) { log(LogLevel::CRITICAL, "[CRITICAL]", msg); }
void Logger::error(const std::string& msg)    { log(LogLevel::ERROR,    "[ERROR]",    msg); }
void Logger::warn(const std::string& msg)     { log(LogLevel::WARN,     "[WARN]",     msg); }
void Logger::info(const std::string& msg)     { log(LogLevel::INFO,     "[INFO]",     msg); }
void Logger::debug(const std::string& msg)    { log(LogLevel::DEBUG,    "[DEBUG]",    msg); }

void Logger::log(LogLevel level, const std::string& levelStr, const std::string& msg) {
    if (level > logLevel_) return;

    std::lock_guard<std::mutex> guard(logMutex_);

    std::ostringstream oss;
    oss << "[" << timestamp() << "] " << levelStr << " " << msg << std::endl;

    if (useFile_) {
        fileStream_ << oss.str();
        fileStream_.flush();
    } else {
        if (level <= LogLevel::ERROR) {
            std::cerr << oss.str();
        } else {
            std::cout << oss.str();
        }
    }
}

std::string Logger::timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now)))
        return buf;
    return "";
}
