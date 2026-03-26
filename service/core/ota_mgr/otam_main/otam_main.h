#ifndef OTAM_MAIN_H_
#define OTAM_MAIN_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "otam_common/otam_service.h"
#include "otam_diagnostic_service/otam_diagnostic_service.h"

class OtamMain {
 public:
  OtamMain();
  ~OtamMain();

  OtamMain(std::vector<std::shared_ptr<OtamService>> services) : units_(std::move(services)) {}

  // Creates and initializes all units in the OTA Manager. [cite: 11]
  void Initialize();

  // Starts the functionality defined in the main class of all units. [cite: 12]
  void Start();

  // Terminates all units and returns resources. [cite: 13]
  void Stop();

 private:
  // Background thread: detects state changes and sends periodic heartbeats.
  void MonitorLoop();

  // Vector to hold pointers to the main classes of other units.
  std::vector<std::shared_ptr<OtamService>> units_;
  std::shared_ptr<OtamDiagnosticService> diagnostic_service_;

  std::thread monitor_thread_;
  std::atomic<bool> monitor_stop_{false};
  std::map<std::string, ServiceState> last_states_;

  static constexpr int kPollIntervalSec      = 5;
  static constexpr int kHeartbeatIntervalSec = 30;
};

#endif  // OTAM_MAIN_H_