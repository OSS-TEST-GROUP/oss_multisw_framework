#ifndef MOCK_NOTIFY_APP_STATE_WRITER_HPP_
#define MOCK_NOTIFY_APP_STATE_WRITER_HPP_

#include "lifecycle/NotifyServiceState.hpp"
#include <gmock/gmock.h>

class MockNotifyAppStateWriter : public adsw_manager::agl::NotifyAppStateWriter
{
public:
    MOCK_METHOD(bool, Initialize,
                (eprosima::fastdds::dds::Publisher *, eprosima::fastdds::dds::DomainParticipant *),
                (override));
    MOCK_METHOD(bool, Publish, (const lifecyclemgr::NotifyServiceState &), (override));
};

#endif