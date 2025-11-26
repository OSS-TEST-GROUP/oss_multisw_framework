#ifndef MOCK_START_APP_HANDLER_HPP_
#define MOCK_START_APP_HANDLER_HPP_

#include <gmock/gmock.h>
#include "StartAppHandler.hpp"

class MockStartAppHandler : public adsw_manager::agl::StartAppHandler {
    public:
        MOCK_METHOD(bool, Initialize,
            (eprosima::fastdds::dds::DomainParticipant*,
            eprosima::fastdds::dds::Publisher*,
            eprosima::fastdds::dds::Subscriber*,
            std::function<void(const lifecyclemgr::StartServiceRequest&)>),
            (override));

        MOCK_METHOD(bool, PublishResponse,
            (const lifecyclemgr::StartServiceResponse&),
            (override));
};

#endif