#ifndef OTAM_LOGGER_H_
#define OTAM_LOGGER_H_

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>

/**
 * @brief Thread-safe tee streambuf.
 *
 * Mirrors every character written to it onto two destinations:
 *   1. The original streambuf (console)
 *   2. An open std::ofstream (log file)
 *
 * A timestamp is prepended at the beginning of each new line.
 */
class OtamTeeBuf : public std::streambuf {
 public:
  OtamTeeBuf(std::streambuf* orig, std::ofstream& file, const std::string& prefix)
      : orig_(orig), file_(file), prefix_(prefix), line_start_(true) {}

 protected:
  int overflow(int c) override {
    if (c == traits_type::eof()) return traits_type::eof();
    std::lock_guard<std::mutex> lock(mutex_);
    WriteChar(static_cast<char>(c));
    return c;
  }

  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::lock_guard<std::mutex> lock(mutex_);
    for (std::streamsize i = 0; i < n; ++i) {
      WriteChar(s[i]);
    }
    return n;
  }

 private:
  void WriteChar(char c) {
    if (line_start_) {
      const std::string ts = Timestamp();
      orig_->sputn(ts.c_str(), static_cast<std::streamsize>(ts.size()));
      if (file_.is_open()) {
        file_.rdbuf()->sputn(ts.c_str(), static_cast<std::streamsize>(ts.size()));
      }
      line_start_ = false;
    }
    orig_->sputc(c);
    if (file_.is_open()) file_.rdbuf()->sputc(c);
    if (c == '\n') line_start_ = true;
  }

  std::string Timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto tt  = std::chrono::system_clock::to_time_t(now);
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                   now.time_since_epoch()) % 1000;
    char buf[24];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
    std::ostringstream oss;
    oss << "[" << buf << "." << std::setfill('0') << std::setw(3) << ms.count()
        << "]" << prefix_;
    return oss.str();
  }

  std::streambuf* orig_;
  std::ofstream&  file_;
  std::string     prefix_;   // e.g. " " for stdout, " [ERR] " for stderr
  bool            line_start_;
  std::mutex      mutex_;
};

/**
 * @brief OtamLogger – file logging for OTA Manager.
 *
 * Redirects std::cout and std::cerr so that every message is written to
 * both the console and a timestamped log file.
 *
 * Usage (in main.cc):
 *   OtamLogger::Init();          // uses OTAM_LOG_FILE env var or default path
 *   ...
 *   OtamLogger::Shutdown();      // flush & restore streams (called by dtor too)
 *
 * Log rotation: when the file exceeds max_bytes the current file is renamed
 * to <path>.1 (previous .1 → .2, up to kMaxRotations).
 */
class OtamLogger {
 public:
  static constexpr const char* kDefaultLogPath  = "logs/otamanager.log";
  static constexpr const char* kEnvVarLogPath   = "OTAM_LOG_FILE";
  static constexpr std::size_t kDefaultMaxBytes = 10UL * 1024 * 1024;  // 10 MiB
  static constexpr int         kMaxRotations    = 3;

  /**
   * @param log_path  Log file path (empty = use OTAM_LOG_FILE env var or default).
   * @param max_bytes Rotate when log file exceeds this size.
   */
  static bool Init(const std::string& log_path = "",
                   std::size_t max_bytes = kDefaultMaxBytes);

  /** Flush, restore original stream buffers, close file. */
  static void Shutdown();

  /** Return true if the logger was successfully initialised. */
  static bool IsInitialized() { return orig_cout_ != nullptr; }

 private:
  static void Rotate(const std::string& path);

  static std::ofstream              log_file_;
  static std::unique_ptr<OtamTeeBuf> cout_buf_;
  static std::unique_ptr<OtamTeeBuf> cerr_buf_;
  static std::streambuf*            orig_cout_;
  static std::streambuf*            orig_cerr_;
};

#endif  // OTAM_LOGGER_H_
