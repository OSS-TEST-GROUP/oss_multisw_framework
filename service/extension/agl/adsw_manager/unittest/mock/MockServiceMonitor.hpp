#ifndef MOCK_SERVICE_MONITOR_HPP_
#define MOCK_SERVICE_MONITOR_HPP_

#include "service_monitor/ServiceMonitor.hpp"
#include <gmock/gmock.h>

using namespace adsw_manager_agl;

class MockServiceMonitor : public adsw_manager::agl::ServiceMonitor {
public:
    MOCK_METHOD(bool, Initialize, (eprosima::fastdds::dds::DomainParticipant*,
                                   std::shared_ptr<adsw_manager::agl::ServiceRegistry>,
                                   std::shared_ptr<adsw_manager::agl::MessageTransmitter>), (override));
    MOCK_METHOD(void, StartMonitoring, (), (override));
    MOCK_METHOD(void, StopMonitoring, (), (override));
};

#endif