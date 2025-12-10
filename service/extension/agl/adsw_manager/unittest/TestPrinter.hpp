#pragma once

#include <ostream>
#include "lifecycle/LifecycleCommon.hpp"

namespace lifecyclemgr {

    inline void PrintTo(const ServiceType& v, std::ostream* os) {
  *os << "ServiceType(" << static_cast<int>(v) << ")";
}
inline void PrintTo(const RecoveryPolicy& v, std::ostream* os) {
  *os << "RecoveryPolicy(" << static_cast<int>(v) << ")";
}
inline void PrintTo(const ReturnCode& v, std::ostream* os) {
  *os << "ReturnCode(" << static_cast<int>(v) << ")";
}
inline void PrintTo(const ServiceMode& v, std::ostream* os) {
  *os << "ServiceMode(" << static_cast<int>(v) << ")";
}
// inline void PrintTo(const ServiceState& v, std::ostream* os) {
//   *os << "ServiceState(" << static_cast<int>(v) << ")";
// }
inline void PrintTo(const OsType& v, std::ostream* os) {
  *os << "OsType(" << static_cast<int>(v) << ")";
}

inline void PrintTo(const ServiceState& state, std::ostream* os) {
  switch (state) {
    case ServiceState::NOT_RUNNING:         *os << "NOT_RUNNING"; break;
    case ServiceState::INITIALIZING:        *os << "INITIALIZING"; break;
    case ServiceState::RUNNING:             *os << "RUNNING"; break;
    case ServiceState::RUNNING_NO_RESPONSE: *os << "RUNNING_NO_RESPONSE"; break;
    case ServiceState::TERMINATING:         *os << "TERMINATING"; break;
    default:
      *os << "ServiceState(" << static_cast<int>(state) << ")";
      break;
  }
}

} // namespace lifecyclemgr
