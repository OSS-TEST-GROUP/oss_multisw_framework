#ifndef DIAGNOSTICS_DIAGNOSTIC_DIAGNOSTIC_PUBLISHER_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_DIAGNOSTIC_PUBLISHER_HPP_

#include <iostream>
#include <memory>

#include "executor/ServiceExecutor.hpp"
#include "transmitter/MessageTransmitter.hpp"
#include "diagnostic/DiagInfoRegistry.hpp"

namespace diagnostic_manager {

class DiagnosticPublisher {
  public:
    DiagnosticPublisher(std::shared_ptr<ServiceExecutor> service_executor, std::shared_ptr<MessageTransmitter> message_transmitter,
        std::shared_ptr<DiagInfoRegistry> diag_info_registry);
    virtual ~DiagnosticPublisher();

    bool Initialize();

    void PublishUpdatedInfo();

  private:
    void HandleDiagInfoMessage(std::shared_ptr<RequestBase> request_msg);
    void HandleRequestGetDiagInfo(std::shared_ptr<VariantData> data, const std::string& request_message_name);
    void HandleRequestGetDiagInfoList(std::shared_ptr<VariantData> data, const std::string& request_message_name);

    void HandlePublicationProcess(std::shared_ptr<RequestBase> request_msg);

    std::shared_ptr<Message> CreateDiagInfoToNotificationMessage(std::shared_ptr<DiagInfo> diag_info);
    std::shared_ptr<VariantData> ConvertDiagInfoToVariantData(std::shared_ptr<DiagInfo> diag_info);

    std::shared_ptr<ServiceExecutor> service_executor_;
    std::shared_ptr<MessageTransmitter> message_transmitter_;
    std::shared_ptr<DiagInfoRegistry> diag_info_registry_;

    std::chrono::steady_clock::time_point last_publish_time_;
    TaskID publisher_task_id_{-1};
    int64_t subscriber_id_{-1};
};

} // namespace diagnostic_manager

#endif
