#ifndef DIAGNOSTICS_TRASMITTER_MESSAGE_HPP_
#define DIAGNOSTICS_TRASMITTER_MESSAGE_HPP_

#include <iostream>
#include "MessageProtocolType.hpp"
#include "diagnostic/VariantData.hpp"

namespace diagnostic_manager {

using MessageTime = std::chrono::steady_clock::time_point;

class Message {
  public:
    Message();
    Message(const std::string& name, MessageProtocolType protocol_type, const std::string& type, std::shared_ptr<VariantData> data,
            const std::string& identifier = "");
    virtual ~Message();

    void SetName(const std::string& name);
    std::string GetName();

    void SetProtocolType(MessageProtocolType type);
    MessageProtocolType GetProtocolType();

    void SetTypeName(const std::string& name);
    std::string GetTypeName();

    void SetData(std::shared_ptr<VariantData> data);
    std::shared_ptr<VariantData> GetData();

    void SetIdentifier(const std::string& identifier);
    std::string GetIdentifier();

    MessageTime GetUpdateTime();

  private:
    std::string name_;
    MessageProtocolType protocol_;
    std::string type_name_;
    std::shared_ptr<VariantData> data_;
    std::string identifier_;
    MessageTime update_time_;
};

} // namespace diagnostic_manager

#endif
