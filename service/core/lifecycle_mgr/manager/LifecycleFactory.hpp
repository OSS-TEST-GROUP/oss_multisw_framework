#ifndef LIFECYCLE_LIFECYCLE_FACTORY_HPP_
#define LIFECYCLE_LIFECYCLE_FACTORY_HPP_

#include <array>
#include <cstdint>
#include <mutex>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include "manager/LifecycleManager.hpp"

namespace lifecycle_manager {


class LifecycleFactory {
  public:
    LifecycleFactory();
    virtual ~LifecycleFactory();

    static LifecycleFactory* GetInstance();

    std::shared_ptr<LifecycleManager> GetManager();

    std::shared_ptr<DDSTransmitter> GetTransmitter();
    std::shared_ptr<StateManager> GetStateMgr();
    std::shared_ptr<StateMonitor> GetMonitor();
    std::shared_ptr<PolicyManager> GetPolicy();
    std::shared_ptr<MWServiceManager> GetMWService();
    std::shared_ptr<ServiceRegistry> GetRegistry();

    // Phantom-publisher guard. lifecycle_mgr learns the GUID prefix of the
    // trusted adsw_manager when it first receives UpdateServiceListRequest —
    // adsw_manager is the only legitimate writer on that topic in the
    // deployment, so pinning that prefix gives us a one-shot identity
    // anchor. Endpoints that receive Start/Stop responses then call
    // IsKnownAdswPublisher() to drop samples coming from any other
    // participant on the same DDS domain (zombie publishers, rogue tools,
    // etc.) before they corrupt OnStart/OnStop's in-flight counters.
    void SetKnownAdswPublisher(
        const eprosima::fastdds::dds::InstanceHandle_t& handle);
    bool IsKnownAdswPublisher(
        const eprosima::fastdds::dds::InstanceHandle_t& handle);
    bool KnownAdswPublisherSet();

  private :
    static std::shared_ptr<LifecycleFactory> getShared();

  private:
    std::shared_ptr<LifecycleManager> lifecycle_manager_;

    // GuidPrefix (first 12 bytes of an InstanceHandle_t) of the participant
    // we trust. Empty until the first UpdateServiceListRequest pins it.
    std::mutex              adsw_prefix_mutex_;
    bool                    adsw_prefix_set_ = false;
    std::array<uint8_t, 12> adsw_prefix_{};
};


} // namespace lifecycle_manager

#endif // LIFECYCLE_LIFECYCLE_FACTORY_HPP_
