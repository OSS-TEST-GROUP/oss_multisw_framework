#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_CONTAINER_HEALTH_MONITOR_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_CONTAINER_HEALTH_MONITOR_HPP_

#include "monitor/ContainerHealthMonitor.hpp"

// No-op ContainerHealthMonitor definitions for unit tests.
//
// F013 added ContainerHealthMonitor, which ADSWManager::Initialize() now
// constructs and Start()s — spinning up a real docker-polling thread. Linking
// the real ContainerHealthMonitor.cpp into ADSWManagerTest would (a) drag in
// the whole ContainerController/Service/DDS graph and (b) run that thread
// against torn-down mocks, crashing the test. Defining the methods as no-ops
// here (the established -fno-access-control mock pattern) keeps the symbols
// resolvable while neutering the background thread. The real behaviour is
// covered by ADSWContainerHealthMonitorTest.
namespace ros2 {
namespace adsw_manager {

ContainerHealthMonitor::ContainerHealthMonitor() {}
ContainerHealthMonitor::~ContainerHealthMonitor() {}
void ContainerHealthMonitor::Start() {}
void ContainerHealthMonitor::Stop() {}
void ContainerHealthMonitor::NotifyInitialContainerState() {}
// virtual — defined so the vtable resolves even though Start() is a no-op
void ContainerHealthMonitor::NotifyContainerState(const std::string&, bool, int32_t) {}
void ContainerHealthMonitor::PublishContainerStateHeartbeat(const std::string&, bool, int32_t) {}

} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_UNITTEST_MOCK_CONTAINER_HEALTH_MONITOR_HPP_
