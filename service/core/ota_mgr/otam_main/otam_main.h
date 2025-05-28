#ifndef OTAM_MAIN_H_
#define OTAM_MAIN_H_

#include <memory>
#include <vector>
#include <string>

#include "otam_common/service.h"

class OtamMain {
 public:
  OtamMain();
  ~OtamMain();

  OtamMain(std::vector<std::shared_ptr<Service>> services) : units_(std::move(services)) {}

  // Creates and initializes all units in the OTA Manager. [cite: 11]
  void Initialize();

  // Starts the functionality defined in the main class of all units. [cite: 12]
  void Start();

  // Terminates all units and returns resources. [cite: 13]
  void Stop();

 private:
  // Vector to hold pointers to the main classes of other units.
  std::vector<std::shared_ptr<Service>> units_;

  // Helper function to handle errors during unit creation/start.
  void HandleUnitError(const std::string& unit_name, const std::string& error_message);
};

#endif  // OTAM_MAIN_H_