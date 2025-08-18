// MockDDSManager.hpp
#include "DDSManager.hpp"
#include <gmock/gmock.h>

using namespace adsw_manager_agl;

class MockAppStateHandler : public adsw_manager::agl::AppStateHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const GetAppStateResponse&), (override));
};
class MockStartAppHandler : public adsw_manager::agl::StartAppHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const StartAppResponse&), (override));
};
class MockStopAppHandler : public adsw_manager::agl::StopAppHandler {
public:
    MOCK_METHOD(bool, PublishResponse, (const StopAppResponse&), (override));
};

class MockDDSManager : public adsw_manager::agl::DDSManager {
public:
    MOCK_METHOD(bool, Initialize, (eprosima::fastdds::dds::DomainParticipant*), (override));
    MOCK_METHOD(void, PublishNotifyAppState, (const NotifyAppState&), (override));
    MOCK_METHOD(void, SetOnGetAppStateRequest, (std::function<void(const GetAppStateRequest&)> cb), (override));
    MOCK_METHOD(void, SetOnStartAppRequest, (std::function<void(const StartAppRequest&)> cb), (override));
    MOCK_METHOD(void, SetOnStopAppRequest, (std::function<void(const StopAppRequest&)> cb), (override));

    MOCK_METHOD(MockAppStateHandler*, GetAppStateHandler, (), (override));
    MOCK_METHOD(MockStartAppHandler*, GetStartAppHandler, (), (override));
    MOCK_METHOD(MockStopAppHandler*, GetStopAppHandler, (), (override));
};
