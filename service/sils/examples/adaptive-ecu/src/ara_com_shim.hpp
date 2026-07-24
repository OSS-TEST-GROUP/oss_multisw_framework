#ifndef ADAPTIVE_ECU_ARA_COM_SHIM_HPP
#define ADAPTIVE_ECU_ARA_COM_SHIM_HPP

// ara_com_shim.hpp -- a MINIMAL teaching stand-in for `ara::com`, the AUTOSAR Adaptive
// Platform communication API. It is NOT a real AP stack: it exists only to show WHERE an
// Adaptive software component (SWC) connects, so the same SWC source that runs on a vendor
// AP runtime (Vector, ETAS, EB, Apex.AI...) also runs here. In production you delete this
// file and link the vendor `ara::com` -- whose SOME/IP or DDS binding does what the
// AdaptiveBehavior adapter does here (deliver subscribed events, dispatch offered events).
//
// Why Adaptive is the natural fit (Case 2 of 3): an AP app is already a POSIX process doing
// service-oriented comms, so there is no RTE/BSW to virtualize (contrast Classic, Case 3).
// The only work is binding ara::com event(s) to the canonical DDS contract -- and ara::com
// itself can be configured with a DDS binding, making this a thin, standards-based seam.

#include <cstdint>
#include <functional>

namespace ara {
namespace com {

// Offered-service event (Skeleton side): the SWC calls Send(); the runtime drains it.
template <typename T>
class SkeletonEvent {
public:
    void Send(const T& v) { last_ = v; has_ = true; }
    // Runtime-facing (the shim/adapter plays the AP runtime): take the latest Send().
    bool TakeLast(T& out) {
        if (!has_) return false;
        out = last_;
        has_ = false;
        return true;
    }

private:
    T last_{};
    bool has_ = false;
};

// Consumed-service event (Proxy side): the runtime Deliver()s a sample; the SWC reacts via
// a receive handler (event-driven), the ara::com idiom.
template <typename T>
class ProxyEvent {
public:
    using Handler = std::function<void(const T&)>;
    void SetReceiveHandler(Handler h) { handler_ = std::move(h); }
    // Runtime-facing: deliver one sample, invoking the SWC's registered handler.
    void Deliver(const T& v) {
        last_ = v;
        has_ = true;
        if (handler_) handler_(v);
    }
    bool GetNewSample(T& out) const {
        if (!has_) return false;
        out = last_;
        return true;
    }

private:
    T last_{};
    bool has_ = false;
    Handler handler_;
};

}  // namespace com
}  // namespace ara

#endif  // ADAPTIVE_ECU_ARA_COM_SHIM_HPP
