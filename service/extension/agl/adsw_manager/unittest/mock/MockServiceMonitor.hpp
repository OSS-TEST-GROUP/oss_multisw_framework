#ifndef MOCK_SERVICE_MONITOR_HPP_
#define MOCK_SERVICE_MONITOR_HPP_

#include "service_monitor/ServiceMonitor.hpp"
#include <gmock/gmock.h>

class MockServiceMonitor : public adsw_manager::agl::ServiceMonitor {
public:
    MOCK_METHOD(bool, Initialize, (eprosima::fastdds::dds::DomainParticipant*,
                                   std::shared_ptr<adsw_manager::agl::ServiceRegistry>,
                                   std::shared_ptr<adsw_manager::agl::MessageTransmitter>,
                                   adsw_manager::agl::AppController*), (override));
    MOCK_METHOD(void, StartMonitoring, (), (override));
    MOCK_METHOD(void, StopMonitoring, (), (override));
};

#endif