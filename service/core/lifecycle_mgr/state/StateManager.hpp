#ifndef LIFECYCLE_STATE_MANAGER_HPP_
#define LIFECYCLE_STATE_MANAGER_HPP_

#include <atomic>
#include <memory>
#include <vector>
#include <set>

#include "manager/ManagerBase.hpp"

#include "policy/PolicyManager.hpp"
#include "include/ServiceMode.hpp"
#include "lifecycle/StartServiceRequest.hpp"
#include "lifecycle/StopServiceRequest.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
// F013
#include "lifecycle/StartContainerRequest.hpp"
#include "lifecycle/StopContainerRequest.hpp"
#include "lifecycle/LifecycleCtrlRequest.hpp"
// F014 follow-up
#include "lifecycle/SetRecoveryPolicyRequest.hpp"

namespace lifecycle_manager {

class StateManager : public ManagerBase {
  public:
    StateManager(LifecycleManager& lifecycle);
    ~StateManager() override= default;
    bool Start();
    void Stop();

    bool StartService(const std::shared_ptr<Service>& service);
    void StopService(const std::shared_ptr<Service>& service);

    bool SetServiceMode(lifecyclemgr::SetServiceModeRequest* message);
    void ProcessServiceHandling(const std::set<std::shared_ptr<Service>>& service_list);

    void RequestStopAll();

    void OnStart(lifecyclemgr::StartServiceResponse* message);
    void OnStop(lifecyclemgr::StopServiceResponse* message);

    // ── F013: actuator response relays ─────────────────────────────────
    // adsw_manager → lifecycle_mgr Start/StopContainerResponse arrives
    // here.  We forward the verdict to the ctrl/* CtrlResponse topic so
    // monitor sees the outcome of its CtrlStart/StopContainer request.
    void OnStartContainer(lifecyclemgr::StartContainerResponse* message);
    void OnStopContainer(lifecyclemgr::StopContainerResponse* message);

    // ── F013: ctrl/* gate handlers ─────────────────────────────────────
    // Each runs lightweight policy gating against the ServiceRegistry,
    // forwards to the corresponding actuator topic on success, and
    // publishes a CtrlResponse with the gating verdict on the matching
    // ctrl/* response topic.  ret_code mirrors lifecyclemgr::ReturnCode.
    void HandleCtrlStartService(lifecyclemgr::CtrlStartServiceRequest* message);
    void HandleCtrlStopService(lifecyclemgr::CtrlStopServiceRequest* message);
    void HandleCtrlSetServiceMode(lifecyclemgr::CtrlSetServiceModeRequest* message);
    void HandleCtrlStartContainer(lifecyclemgr::CtrlStartContainerRequest* message);
    void HandleCtrlStopContainer(lifecyclemgr::CtrlStopContainerRequest* message);

    // F014 follow-up: recovery policy mutation
    void HandleCtrlSetRecoveryPolicy(lifecyclemgr::CtrlSetRecoveryPolicyRequest* message);
    void OnSetRecoveryPolicy(lifecyclemgr::SetRecoveryPolicyResponse* message);

  private:
    // F013: publish a CtrlResponse on the given ctrl/* response topic.
    bool PublishCtrlResponse(MessageType response_topic,
                              lifecyclemgr::ReturnCode code);

  private:
    std::shared_ptr<PolicyManager> policy_manager_;
    lifecyclemgr::StartServiceRequest start_request_;
    lifecyclemgr::StopServiceRequest stop_request_;
    lifecyclemgr::SetServiceModeResponse set_response_;

    // F013: scratch publish buffers — reused per call, single-thread on the
    // ctrl/sub dispatch path.
    lifecyclemgr::SetServiceModeRequest   set_mode_request_;
    lifecyclemgr::StartContainerRequest   start_container_request_;
    lifecyclemgr::StopContainerRequest    stop_container_request_;
    lifecyclemgr::CtrlResponse            ctrl_response_;
    // F014 follow-up
    lifecyclemgr::SetRecoveryPolicyRequest set_recovery_request_;

    // In-flight counter for Start/Stop service requests. Used to gate
    // OnStart/OnStop against phantom StartServiceResponse / StopServiceResponse
    // samples — e.g. zombie DDS publishers replaying stale FAILED responses on
    // the response topic when we did not actually send a request. Incremented
    // when we publish a request to adsw_manager, decremented when we accept a
    // response. Atomic because the actuator response arrives on the DDS
    // listener thread while the request is dispatched from the ctrl handler /
    // policy thread.
    std::atomic<int> start_in_flight_{0};
    std::atomic<int> stop_in_flight_{0};
};

} // namespace lifecycle_manager

#endif // LIFECYCLE_STATE_MANAGER_HPP_
