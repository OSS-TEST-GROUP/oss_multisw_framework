#ifndef MOCK_UPDATE_APP_LIST_HANDLER_HPP_
#define MOCK_UPDATE_APP_LIST_HANDLER_HPP_

#include <gmock/gmock.h>
#include "UpdateAppListHandler.hpp"

class MockUpdateAppListHandler : public adsw_manager::agl::UpdateAppListHandler {
public:
    MOCK_METHOD(bool, Initialize,
        (eprosima::fastdds::dds::DomainParticipant*,
         eprosima::fastdds::dds::Publisher*,
         eprosima::fastdds::dds::Subscriber*,
         std::function<void(const lifecyclemgr::UpdateServiceListResponse&)>),
        (override));

    MOCK_METHOD(bool, Publish,
        (const lifecyclemgr::UpdateServiceListRequest&),
        (override));
};

#endif