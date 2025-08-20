#include "DDSManager.hpp"
#include <gmock/gmock.h>

using namespace lifecyclemgr;

class MockAppStateHandler : public adsw_manager::agl::AppStateHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const GetServiceStateResponse&), (override));
};
class MockStartAppHandler : public adsw_manager::agl::StartAppHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const StartServiceResponse&), (override));
};
class MockStopAppHandler : public adsw_manager::agl::StopAppHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const StopServiceResponse&), (override));
};

class MockDDSManager : public adsw_manager::agl::DDSManager {
public:
    MOCK_METHOD(bool, Initialize, (eprosima::fastdds::dds::DomainParticipant*), (override));
    MOCK_METHOD(void, PublishNotifyAppState, (const NotifyServiceState&), (override));
    MOCK_METHOD(void, SetOnGetAppStateRequest, (std::function<void(const GetServiceStateRequest&)> cb), (override));
    MOCK_METHOD(void, SetOnStartAppRequest, (std::function<void(const StartServiceRequest&)> cb), (override));
    MOCK_METHOD(void, SetOnStopAppRequest, (std::function<void(const StopServiceRequest&)> cb), (override));

    MOCK_METHOD(MockAppStateHandler*, GetAppStateHandler, (), (override));
    MOCK_METHOD(MockStartAppHandler*, GetStartAppHandler, (), (override));
    MOCK_METHOD(MockStopAppHandler*, GetStopAppHandler, (), (override));
};
