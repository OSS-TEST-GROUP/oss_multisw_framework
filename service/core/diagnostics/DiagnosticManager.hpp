#ifndef DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_

#include "executor/ServiceExecutor.hpp"
#include "transmitter/MessageTransmitter.hpp"
#include "diagnostic/DiagInfoRegistry.hpp"

namespace diagnostic_manager {

class DiagnosticManager {
  public:
    DiagnosticManager();
    virtual ~DiagnosticManager();

    bool Initialize();

    bool Start();
    void Stop();

  private:
    std::shared_ptr<ServiceExecutor> service_executor_;
    std::shared_ptr<MessageTransmitter> message_transmitter_;
    std::shared_ptr<DiagInfoRegistry> diag_info_registry_;
};

} // namespace diagnostic_manager

#endif
