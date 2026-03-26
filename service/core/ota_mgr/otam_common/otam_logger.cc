#include "otam_logger.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
std::ofstream              OtamLogger::log_file_;
std::unique_ptr<OtamTeeBuf> OtamLogger::cout_buf_;
std::unique_ptr<OtamTeeBuf> OtamLogger::cerr_buf_;
std::streambuf*            OtamLogger::orig_cout_{nullptr};
std::streambuf*            OtamLogger::orig_cerr_{nullptr};

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
bool OtamLogger::Init(const std::string& log_path, std::size_t max_bytes) {
  if (IsInitialized()) return true;  // already running

  // Resolve log file path: argument > env var > default
  std::string path = log_path;
  if (path.empty()) {
    const char* env = std::getenv(kEnvVarLogPath);
    path = env ? env : kDefaultLogPath;
  }

  // Create parent directory if it does not exist
  const auto dir = std::filesystem::path(path).parent_path();
  if (!dir.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
      std::cerr << "OtamLogger: cannot create log directory '" << dir.string()
                << "': " << ec.message() << std::endl;
      return false;
    }
  }

  // Rotate if the existing file is already over the size limit
  Rotate(path);

  log_file_.open(path, std::ios::app);
  if (!log_file_.is_open()) {
    std::cerr << "OtamLogger: cannot open log file '" << path << "'" << std::endl;
    return false;
  }

  // Redirect cout (plain " " prefix) and cerr (" [ERR] " prefix)
  orig_cout_ = std::cout.rdbuf();
  orig_cerr_ = std::cerr.rdbuf();

  cout_buf_ = std::make_unique<OtamTeeBuf>(orig_cout_, log_file_, " ");
  cerr_buf_ = std::make_unique<OtamTeeBuf>(orig_cerr_, log_file_, " [ERR] ");

  std::cout.rdbuf(cout_buf_.get());
  std::cerr.rdbuf(cerr_buf_.get());

  std::cout << "OtamLogger: logging started -> " << path << std::endl;
  return true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void OtamLogger::Shutdown() {
  if (!IsInitialized()) return;

  std::cout << "OtamLogger: logging stopped." << std::endl;

  std::cout.rdbuf(orig_cout_);
  std::cerr.rdbuf(orig_cerr_);
  orig_cout_ = nullptr;
  orig_cerr_ = nullptr;

  cout_buf_.reset();
  cerr_buf_.reset();

  if (log_file_.is_open()) {
    log_file_.flush();
    log_file_.close();
  }
}

// ---------------------------------------------------------------------------
// Rotate
// ---------------------------------------------------------------------------
void OtamLogger::Rotate(const std::string& path) {
  std::error_code ec;
  const auto size = std::filesystem::file_size(path, ec);
  if (ec || size == 0) return;  // file does not exist or is empty – nothing to rotate

  // Shift old rotations: .3 is dropped, .2→.3, .1→.2, current→.1
  for (int i = kMaxRotations - 1; i >= 1; --i) {
    const std::string older  = path + "." + std::to_string(i);
    const std::string newer  = path + "." + std::to_string(i + 1);
    if (std::filesystem::exists(older, ec)) {
      std::filesystem::rename(older, newer, ec);
    }
  }
  std::filesystem::rename(path, path + ".1", ec);
}
