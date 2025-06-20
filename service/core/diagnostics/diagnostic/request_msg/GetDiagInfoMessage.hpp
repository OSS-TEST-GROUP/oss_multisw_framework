#ifndef DIAGNOSTICS_DIAGNOSTIC_GET_DIAG_INFO_MESSAGE_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_GET_DIAG_INFO_MESSAGE_HPP_

#include <iostream>
#include <memory>

#include "transmitter/request_msg/IncomingMessageBase.hpp"

namespace diagnostic_manager {

class GetDiagInfoMessage : public IncomingMessageBase {};

} // namespace diagnostic_manager

#endif
