#include <iostream>
#include <stdexcept> // For std::runtime_error

#include "otam_main.h"
#include "otam_client_service/otam_client_service.h"

// Dummy implementations of other services for compilation.
// These will be replaced by actual implementations when those units are developed.
class OtamServerService : public Service {
 public:
  void Initialize() override { std::cout << "OtamServerService Initialized." << std::endl; }
  void Start() override { std::cout << "OtamServerService Started." << std::endl; }
  void Stop() override { std::cout << "OtamServerService Stopped." << std::endl; }
};

class OtamVehicleService : public Service {
 public:
  void Initialize() override { std::cout << "OtamVehicleService Initialized." << std::endl; }
  void Start() override { std::cout << "OtamVehicleService Started." << std::endl; }
  void Stop() override { std::cout << "OtamVehicleService Stopped." << std::endl; }
};

class OtamDiagnosticService : public Service {
 public:
  void Initialize() override { std::cout << "OtamDiagnosticService Initialized." << std::endl; }
  void Start() override { std::cout << "OtamDiagnosticService Started." << std::endl; }
  void Stop() override { std::cout << "OtamDiagnosticService Stopped." << std::endl; }

  // Assumes OtamDiagnosticService has a method to update error status.
  void UpdateErrorStatus(const std::string& error_message) {
    std::cerr << "DiagnosticService reporting error: " << error_message << std::endl;
  }
};

class OtamActorService : public Service {
 public:
  void Initialize() override { std::cout << "OtamActorService Initialized." << std::endl; }
  void Start() override { std::cout << "OtamActorService Started." << std::endl; }
  void Stop() override { std::cout << "OtamActorService Stopped." << std::endl; }
};

OtamMain::OtamMain() {
  // Constructor: Initializes the shared pointers for each service.
  // In a real application, you might use a factory or dependency injection.
  units_.push_back(std::make_shared<OtamServerService>());
  units_.push_back(std::make_shared<OtamClientService>());
  units_.push_back(std::make_shared<OtamVehicleService>());
  units_.push_back(std::make_shared<OtamDiagnosticService>());
  units_.push_back(std::make_shared<OtamActorService>());
}

OtamMain::~OtamMain() {
  // Destructor.
}

void OtamMain::Initialize() {
  std::cout << "OtamMain: Initializing all units." << std::endl;
  for (const auto& unit : units_) {
    try {
      unit->Initialize(); // Creates and initializes all units.
    } catch (const std::exception& e) {
      HandleUnitError("initialization of a unit", e.what());
    }
  }
}

void OtamMain::Start() {
  std::cout << "OtamMain: Starting all units." << std::endl;
  for (const auto& unit : units_) {
    try {
      unit->Start(); // Starts functionality of all units.
    } catch (const std::exception& e) {
      HandleUnitError("start of a unit", e.what());
    }
  }
}

void OtamMain::Stop() {
  std::cout << "OtamMain: Stopping all units." << std::endl;
  for (const auto& unit : units_) {
    try {
      unit->Stop(); // Terminates all units and returns resources.
    } catch (const std::exception& e) {
      // For Stop, just report the error and continue stopping other units.
      std::cerr << "Error during stop of a unit: " << e.what() << std::endl;
    }
  }
}

void OtamMain::HandleUnitError(const std::string& unit_name,
                               const std::string& error_message) {
  std::cerr << "Error during " << unit_name << ": " << error_message << std::endl;
  // Error handling policy: re-creation and re-start upon error during unit creation/start.
  // If OtamDiagnosticService is successfully started, update and transmit the error status
  // through OtamDiagnosticService.

  // In a real scenario, you'd try to re-create and re-start the specific unit.
  // For now, we'll just log and potentially notify the diagnostic service.

  // Check if Diagnostic Service is available and successfully started (simplified for this example).
  for (const auto& unit : units_) {
    if (auto diagnostic_service = std::dynamic_pointer_cast<OtamDiagnosticService>(unit)) {
      diagnostic_service->UpdateErrorStatus(
          "Failed to " + unit_name + ". Error: " + error_message);
      break;
    }
  }
}