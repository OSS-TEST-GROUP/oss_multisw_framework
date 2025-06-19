#ifndef DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_MANAGER_HPP_

#include "executor/ServiceExecutor.hpp"
#include "transmitter/MessageTransmitter.hpp"
#include "diagnostic/DiagnosticPublisher.hpp"
#include "diagnostic/DiagInfoRegistry.hpp"
#include "diagnostic_analyzer/DiagnosticAnalyzer.hpp"

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
    std::shared_ptr<DiagnosticAnalyzer> diagnostic_analyzer_;
    std::shared_ptr<DiagnosticPublisher> diagnostic_publisher_;
};

} // namespace diagnostic_manager

#endif
