#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_REFERENCE_MESSAGE_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_REFERENCE_MESSAGE_HPP_

#include <iostream>
#include <set>

#include "transmitter/MessageProtocolType.hpp"

namespace diagnostic_manager {

class ReferenceMessage {
  public:
    ReferenceMessage(const std::string& name, const std::string& message, MessageProtocolType protocol);
    virtual ~ReferenceMessage();

    std::string GetName();
    std::string GetMessage();
    MessageProtocolType GetProtocol();

    void SetDiagCode(int32_t code);
    void RemoveDiagCode(int32_t code);

    bool CheckDiagCode(int32_t code);
    std::set<int32_t> GetAllDiagCode();

  private:
    std::string name_;
    std::string message_;
    MessageProtocolType protocol_;
    std::set<int32_t> diag_codes_;
};

} // namespace diagnostic_manager

#endif
