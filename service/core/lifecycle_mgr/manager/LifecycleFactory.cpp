#include "LifecycleFactory.hpp"
#include "logger/Log.hpp"

namespace lifecycle_manager {

LifecycleFactory::LifecycleFactory() :lifecycle_manager_(nullptr)
{
    LOG_INFO("Create LifecycleFactory");
}

LifecycleFactory::~LifecycleFactory() {}

LifecycleFactory* LifecycleFactory::GetInstance(){
   return getShared().get();
}

std::shared_ptr<LifecycleFactory> LifecycleFactory::getShared()
{
    // Note we need a custom deleter, since the destructor is protected.
    static std::shared_ptr<LifecycleFactory> instance(
        new LifecycleFactory(),
        [](LifecycleFactory* p)
        {
            delete p;
        });
    return instance;
}

std::shared_ptr<LifecycleManager> LifecycleFactory::GetManager(){
    
    if (lifecycle_manager_ == nullptr){
        lifecycle_manager_ = std::make_shared<LifecycleManager>();
    }
    return lifecycle_manager_;

}

std::shared_ptr<DDSTransmitter> LifecycleFactory::GetTransmitter(){
    return GetManager()->GetTransmitter();
}

std::shared_ptr<StateManager> LifecycleFactory::GetStateMgr(){
    return GetManager()->GetStateMgr();
}

std::shared_ptr<StateMonitor> LifecycleFactory::GetMonitor(){
    return GetManager()->GetMonitor();
}

std::shared_ptr<PolicyManager> LifecycleFactory::GetPolicy(){
    return GetManager()->GetPolicy();
}
std::shared_ptr<MWServiceManager> LifecycleFactory::GetMWService(){
    return GetManager()->GetMWService();
}  

std::shared_ptr<ServiceRegistry> LifecycleFactory::GetRegistry(){
    return GetManager()->GetRegistry();
}

// InstanceHandle_t holds a 16-byte raw value; the first 12 bytes are the
// participant GUID prefix. We treat the prefix as the identity for our
// phantom-publisher guard since a single participant publishes both
// UpdateServiceListRequest (the trust anchor) and the Start/Stop responses
// we want to validate, via different writers (different last 4 bytes).
namespace {
constexpr std::size_t kGuidPrefixBytes = 12;

std::array<uint8_t, kGuidPrefixBytes> make_prefix(
    const eprosima::fastdds::dds::InstanceHandle_t& handle) {
    std::array<uint8_t, kGuidPrefixBytes> out{};
    const uint8_t* raw =
        reinterpret_cast<const uint8_t*>(&handle);
    std::copy_n(raw, kGuidPrefixBytes, out.begin());
    return out;
}
} // namespace

void LifecycleFactory::SetKnownAdswPublisher(
    const eprosima::fastdds::dds::InstanceHandle_t& handle) {
    std::lock_guard<std::mutex> lk(adsw_prefix_mutex_);
    // Trust EVERY participant that sends UpdateServiceListRequest, not just
    // the first one. UpdateServiceListRequest is adsw_manager's deliberate
    // registration handshake, so a phantom/zombie publisher never sends it —
    // accumulating the set keeps the anti-phantom guarantee while fixing two
    // things the old first-writer-wins pin broke:
    //   (1) producer restart — a restarted adsw_manager comes back with a NEW
    //       participant prefix; the old pin would reject all its
    //       NotifyServiceState forever (service stuck NOT_RUNNING on the
    //       monitor), recoverable only by restarting lifecycle_mgr.
    //   (2) multiple adsw_managers (ros2 + agl) feeding one lifecycle_mgr —
    //       the old pin trusted one and dropped the other's state.
    if (known_adsw_prefixes_.insert(make_prefix(handle)).second) {
        LOG_DEBUG("LifecycleFactory: trusting adsw_manager publisher prefix "
                  "(known=" << known_adsw_prefixes_.size() << ")");
    }
}

bool LifecycleFactory::IsKnownAdswPublisher(
    const eprosima::fastdds::dds::InstanceHandle_t& handle) {
    std::lock_guard<std::mutex> lk(adsw_prefix_mutex_);
    // Strict: nothing is trusted until the first UpdateServiceListRequest
    // arrives. The earlier permissive policy let phantom Start/Stop
    // responses sneak through during the boot window, which corrupted
    // OnStart's in-flight counter and surfaced bogus FAILED verdicts to
    // the monitor. adsw_manager publishes UpdateServiceListRequest very
    // early in its own boot (right after CreateEndPoints), so no
    // legitimate response can arrive before the trust anchor is in place.
    if (known_adsw_prefixes_.empty()) return false;
    return known_adsw_prefixes_.count(make_prefix(handle)) > 0;
}

bool LifecycleFactory::KnownAdswPublisherSet() {
    std::lock_guard<std::mutex> lk(adsw_prefix_mutex_);
    return !known_adsw_prefixes_.empty();
}


} // namespace lifecycle_manager
