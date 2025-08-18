#ifndef MOCK_NOTIFY_APP_STATE_WRITER_HPP_
#define MOCK_NOTIFY_APP_STATE_WRITER_HPP_

#include "NotifyAppState.hpp"
#include <gmock/gmock.h>

class MockNotifyAppStateWriter : public adsw_manager::agl::NotifyAppStateWriter {
    public:
        MOCK_METHOD(bool, Publish, (const adsw_manager_agl::NotifyAppState&), (override));
};

#endif