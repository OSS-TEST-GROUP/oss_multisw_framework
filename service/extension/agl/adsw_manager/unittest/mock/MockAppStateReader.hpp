#ifndef MOCK_APP_STATE_READER_HPP_
#define MOCK_APP_STATE_READER_HPP_

#include "service_monitor/AppStateReader.hpp"

using namespace lifecyclemgr;

class MockAppStateReader : public adsw_manager::agl::AppStateReader {
public:
    MockAppStateReader(std::chrono::steady_clock::time_point fake_time)
        : adsw_manager::agl::AppStateReader(), fake_time_(fake_time) {}

    std::chrono::steady_clock::time_point GetLastReceivedTime() const override {
        return fake_time_;
    }

private:
    std::chrono::steady_clock::time_point fake_time_;
};

#endif