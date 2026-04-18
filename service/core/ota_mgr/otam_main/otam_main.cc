#include <chrono>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "otam_main.h"
#include "otam_client_service/otam_client_service.h"
#include "otam_diagnostic_service/otam_diagnostic_service.h"
#include "otam_server_service/otam_server_service.h"
#include "otam_vehicle_service/otam_vehicle_service.h"
#include "otam_actor_service/otam_actor_service.h"

// Adapter to make OtamActorService compatible with Service interface
class OtamActorServiceAdapter : public OtamService {
 public:
  OtamActorServiceAdapter() : actor_service_(std::make_shared<OtamActorService>()) {}

  void Initialize() override {
    actor_service_->Initialize();
  }

  void Start() override {
    actor_service_->Start();
  }

  void Stop() override {
    actor_service_->Stop();
  }

  ServiceState State() override { 
    return actor_service_->State(); 
  }
  std::string Name() override { 
    return actor_service_->Name(); 
  }

  std::shared_ptr<OtamActorService> GetActorService() {
    return actor_service_;
  }

 private:
  std::shared_ptr<OtamActorService> actor_service_;
};

OtamMain::OtamMain() {
  // Constructor: Initializes the shared pointers for each service.
  // In a real application, you might use a factory or dependency injection.
  diagnostic_service_ = std::make_shared<OtamDiagnosticService>();
  auto server_service = std::make_shared<OtamServerService>();
  auto client_service = std::make_shared<OtamClientService>();
  auto vehicle_service = std::make_shared<OtamVehicleService>();
  auto actor_service_adapter = std::make_shared<OtamActorServiceAdapter>();

  // Connect OtamActorService with all required services
  actor_service_adapter->GetActorService()->SetOtamDiagnosticService(diagnostic_service_.get());
  actor_service_adapter->GetActorService()->SetOtamServerService(server_service.get());
  actor_service_adapter->GetActorService()->SetOtamClientService(client_service.get());
  actor_service_adapter->GetActorService()->SetOtamVehicleService(vehicle_service.get());

  // Connect diagnostic service to other services for error reporting
  client_service->SetOtamDiagnosticService(diagnostic_service_.get());
  vehicle_service->SetOtamDiagnosticService(diagnostic_service_.get());
  server_service->SetOtamDiagnosticService(diagnostic_service_.get());

  units_.push_back(server_service);
  units_.push_back(client_service);
  units_.push_back(vehicle_service);
  units_.push_back(actor_service_adapter);
}

OtamMain::~OtamMain() {
  // Destructor.
}

void OtamMain::Initialize() {
  std::cout << "OtamMain: Initializing all units." << std::endl;

  // diagnostic_service_ is null when the vector constructor is used (test mode).
  if (diagnostic_service_) {
    diagnostic_service_->Initialize();
    diagnostic_service_->Start();
  }

  for (const auto& unit : units_) {
    try {
      unit->Initialize(); // Creates and initializes all units.
      if (diagnostic_service_) {
        std::string err_msg = unit->Name() + " " + ServiceStateToString(unit->State());
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::SUCCEEDED), err_msg);
      }
    } catch (const std::exception& e) {
      std::string err_msg = "Failed to initialize " + unit->Name() + ": " + e.what();
      std::cerr << "OtamMain: " << err_msg << std::endl;
      if (diagnostic_service_) {
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
      }
    }
  }
}

void OtamMain::Start() {
  std::cout << "OtamMain: Starting all units." << std::endl;
  for (const auto& unit : units_) {
    try {
      unit->Start(); // Starts functionality of all units.
      if (diagnostic_service_) {
        std::string err_msg = unit->Name() + " " + ServiceStateToString(unit->State());
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::SUCCEEDED), err_msg);
      }
    } catch (const std::exception& e) {
      std::string err_msg = "Failed to start " + unit->Name() + ": " + e.what();
      std::cerr << "OtamMain: " << err_msg << std::endl;
      if (diagnostic_service_) {
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
      }
    }
  }

  // Snapshot initial states and launch monitor thread.
  if (diagnostic_service_) {
    for (const auto& unit : units_) {
      last_states_[unit->Name()] = unit->State();
    }
    monitor_stop_.store(false);
    monitor_thread_ = std::thread(&OtamMain::MonitorLoop, this);
    std::cout << "OtamMain: State monitor thread started." << std::endl;
  }
}

void OtamMain::MonitorLoop() {
  int seconds_since_heartbeat = 0;

  while (!monitor_stop_.load()) {
    for (int i = 0; i < kPollIntervalSec && !monitor_stop_.load(); ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (monitor_stop_.load()) break;

    seconds_since_heartbeat += kPollIntervalSec;
    bool heartbeat_due = (seconds_since_heartbeat >= kHeartbeatIntervalSec);

    for (const auto& unit : units_) {
      ServiceState current = unit->State();
      ServiceState last    = last_states_[unit->Name()];
      bool changed         = (current != last);

      if (changed || heartbeat_due) {
        int32_t err_code = (current == ServiceState::ERROR)
            ? static_cast<int32_t>(OtamError::FAILED)
            : static_cast<int32_t>(OtamError::SUCCEEDED);
        std::string msg = unit->Name() + " " + ServiceStateToString(current);
        if (changed) {
          msg += " (changed from " + ServiceStateToString(last) + ")";
        }
        diagnostic_service_->PublishOtaMgrInfo(err_code, msg);
        if (changed) {
          last_states_[unit->Name()] = current;
        }
      }
    }

    if (heartbeat_due) {
      seconds_since_heartbeat = 0;
    }
  }
}

void OtamMain::Stop() {
  // Stop monitor thread before tearing down units.
  if (monitor_thread_.joinable()) {
    monitor_stop_.store(true);
    monitor_thread_.join();
    std::cout << "OtamMain: State monitor thread stopped." << std::endl;
  }

  std::cout << "OtamMain: Stopping all units." << std::endl;
  for (const auto& unit : units_) {
    try {
      unit->Stop(); // Terminates all units and returns resources.
      if (diagnostic_service_) {
        std::string err_msg = unit->Name() + " " + ServiceStateToString(unit->State());
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::SUCCEEDED), err_msg);
      }
    } catch (const std::exception& e) {
      std::string err_msg = "Failed to stop " + unit->Name() + ": " + e.what();
      fprintf(stderr, "OtamMain: %s\n", err_msg.c_str());
      if (diagnostic_service_) {
        diagnostic_service_->PublishOtaMgrInfo(static_cast<int32_t>(OtamError::FAILED), err_msg);
      }
    }
  }
  if (diagnostic_service_) {
    diagnostic_service_->Stop();
  }
}
