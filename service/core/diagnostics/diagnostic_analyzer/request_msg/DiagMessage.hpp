#ifndef DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_MESSAGE_HPP_
#define DIAGNOSTICS_DIAGNOSTIC_ANALYZER_DIAG_MESSAGE_HPP_

#include <iostream>
#include <memory>

#include "transmitter/request_msg/IncomingMessageBase.hpp"

namespace diagnostic_manager {

class DiagMessage : public IncomingMessageBase {};

} // namespace diagnostic_manager

#endif
