#include "DiagnosticPublisher.hpp"

#include <chrono>
#include "request_msg/GetDiagInfoMessage.hpp"
#include "logger/Log.hpp"

using namespace std::placeholders;

namespace diagnostic_manager {

DiagnosticPublisher::DiagnosticPublisher(std::shared_ptr<ServiceExecutor> service_executor, std::shared_ptr<MessageTransmitter> message_transmitter,
        std::shared_ptr<DiagInfoRegistry> diag_info_registry) :
            service_executor_(service_executor), message_transmitter_(message_transmitter), diag_info_registry_(diag_info_registry) {
    last_publish_time_ = std::chrono::steady_clock::time_point::min();
}
DiagnosticPublisher::~DiagnosticPublisher() {}

bool DiagnosticPublisher::Initialize() {
    subscriber_id_ = message_transmitter_->AddSubscriber("DiagnosticPublisher", std::bind(&DiagnosticPublisher::HandleDiagInfoMessage, this, _1), []() {
        return std::make_shared<GetDiagInfoMessage>();
    });
    if (subscriber_id_ < 0) {
        LOG_ERROR("Failed to register to the MessageTransmitter.");
        return false;
    }
    message_transmitter_->SubscribeMessage(subscriber_id_, "mmf/diagnosticmgr/DiagnosticInfoRequest", MessageProtocolType::DDS, true); // need to ros forward
    message_transmitter_->SubscribeMessage(subscriber_id_, "mmf/diagnosticmgr/DiagnosticInfoListRequest", MessageProtocolType::DDS, true); // need to ros forward

    publisher_task_id_ = service_executor_->AddTaskInterval("DiagnosticAnalyzerRequest", std::bind(&DiagnosticPublisher::HandlePublicationProcess, this, _1), TaskInterval(1000));
    if (publisher_task_id_ < 0) {
        LOG_ERROR("Failed to create task for publishing diag info.");
    }

    return true;
}

void DiagnosticPublisher::PublishUpdatedInfo() {}

void DiagnosticPublisher::HandleDiagInfoMessage(std::shared_ptr<RequestBase> request_msg) {
    static std::map<std::string, std::function<void(std::shared_ptr<VariantData>, const std::string&)>> handler_map = {
        {"mmf/diagnosticmgr/DiagnosticInfoRequest", std::bind(&DiagnosticPublisher::HandleRequestGetDiagInfo, this, _1, _2)},
        {"mmf/diagnosticmgr/DiagnosticInfoListRequest", std::bind(&DiagnosticPublisher::HandleRequestGetDiagInfoList, this, _1, _2)},
        // ROS message handlers with "rt/" prefix
        {"rt/mmf/diagnosticmgr/DiagnosticInfoRequest", std::bind(&DiagnosticPublisher::HandleRequestGetDiagInfo, this, _1, _2)},
        {"rt/mmf/diagnosticmgr/DiagnosticInfoListRequest", std::bind(&DiagnosticPublisher::HandleRequestGetDiagInfoList, this, _1, _2)},
    };

    std::shared_ptr<GetDiagInfoMessage> diag_req = std::dynamic_pointer_cast<GetDiagInfoMessage>(request_msg);
    std::shared_ptr<Message> diag_req_msg = diag_req->GetMessage();
    std::shared_ptr<VariantData> diag_req_data = diag_req_msg->GetData();

    std::string message_name = diag_req_msg->GetName();
    
    auto func = handler_map.find(message_name);
    if (func == handler_map.end()) {
        LOG_ERROR("Not supported request. " << message_name);
        return;
    }

    LOG_INFO("Handling message: " << message_name);
    func->second(diag_req_data, message_name);
}

void DiagnosticPublisher::HandleRequestGetDiagInfo(std::shared_ptr<VariantData> data, const std::string& request_message_name) {
    std::shared_ptr<DiagInfo> diag_info;

    diag_info = diag_info_registry_->GetDiagInfo(data->GetValue<int32_t>("diag_code"));
    if (diag_info == nullptr) {
        diag_info = diag_info_registry_->GetDiagInfoByName(data->GetValue<std::string>("diag_name"));
    }

    std::shared_ptr<VariantData> msg_data = std::make_shared<VariantData>();
    msg_data->SetName("DiagnosticInfoResponse");
    if (diag_info) {
        msg_data->SetValue("diag_code", diag_info->GetDiagCode());
        msg_data->SetValue("diag_name", diag_info->GetDiagName());
        msg_data->SetValue("status", (int32_t)diag_info->GetStatus());
        msg_data->SetValue("message", diag_info->GetMessage());

        msg_data->SetValue("ret_code", (int32_t)1L);
    } else {
        msg_data->SetValue("ret_code", (int32_t)2L);
    }

    // Determine response message name and protocol type based on request message type
    std::string response_message_name = "mmf/diagnosticmgr/DiagnosticInfoResponse";
    MessageProtocolType response_protocol_type = MessageProtocolType::DDS;
    
    if (request_message_name.find("rt/") == 0) {
        response_message_name = "rt/" + response_message_name;
        response_protocol_type = MessageProtocolType::DDS_ROS;
        LOG_INFO("Sending ROS response for DiagnosticInfoResponse");
    }

    std::shared_ptr<Message> msg = std::make_shared<Message>(
        response_message_name,
        MessageProtocolType::DDS,
        msg_data->GetName(),
        msg_data
    );

    message_transmitter_->PublishMessage(msg);
}

void DiagnosticPublisher::HandleRequestGetDiagInfoList(std::shared_ptr<VariantData> data, const std::string& request_message_name) {
    auto diag_codes = diag_info_registry_->GetDiagInfoCodeList();
    auto diag_names = diag_info_registry_->GetDiagInfoNameList();

    std::shared_ptr<VariantData> msg_data = std::make_shared<VariantData>();
    msg_data->SetName("DiagnosticInfoListResponse");
    msg_data->SetValue("diag_codes", diag_codes);
    msg_data->SetValue("diag_names", diag_names);

    msg_data->SetValue("ret_code", (int32_t)1L);

    // Determine response message name and protocol type based on request message type
    std::string response_message_name = "mmf/diagnosticmgr/DiagnosticInfoListResponse";
    MessageProtocolType response_protocol_type = MessageProtocolType::DDS;
    
    if (request_message_name.find("rt/") == 0) {
        response_message_name = "rt/" + response_message_name;
        response_protocol_type = MessageProtocolType::DDS_ROS;
        LOG_INFO("Sending ROS response for DiagnosticInfoListResponse");
    }

    std::shared_ptr<Message> msg = std::make_shared<Message>(
        response_message_name,
        MessageProtocolType::DDS,
        msg_data->GetName(),
        msg_data
    );

    message_transmitter_->PublishMessage(msg);
}

void DiagnosticPublisher::HandlePublicationProcess(std::shared_ptr<RequestBase> request_msg) {
    for (auto iter : diag_info_registry_->GetAllDiagInfo()) {
        if (iter->GetUpdateTime() >= last_publish_time_) {
            std::shared_ptr<Message> diag_msg = CreateDiagInfoToNotificationMessage(iter);
            message_transmitter_->PublishMessage(diag_msg);
        }
    }

    last_publish_time_ = std::chrono::steady_clock::now();
}

std::shared_ptr<Message> DiagnosticPublisher::CreateDiagInfoToNotificationMessage(std::shared_ptr<DiagInfo> diag_info) {
    std::shared_ptr<VariantData> msg_data = ConvertDiagInfoToVariantData(diag_info);

    std::shared_ptr<Message> msg = std::make_shared<Message>(
            "mmf/diagnosticmgr/DiagnosticNotification",
            MessageProtocolType::DDS,
            msg_data->GetName(),
            msg_data
    );

    return msg;
}

std::shared_ptr<VariantData> DiagnosticPublisher::ConvertDiagInfoToVariantData(std::shared_ptr<DiagInfo> diag_info) {
    std::shared_ptr<VariantData> msg_data = std::make_shared<VariantData>();
    msg_data->SetName("DiagnosticNotification");
    msg_data->SetValue("diag_code", diag_info->GetDiagCode());
    msg_data->SetValue("diag_name", diag_info->GetDiagName());
    msg_data->SetValue("status", (int32_t)diag_info->GetStatus());
    msg_data->SetValue("message", diag_info->GetMessage());

    return msg_data;
}

} // namespace diagnostic_manager
