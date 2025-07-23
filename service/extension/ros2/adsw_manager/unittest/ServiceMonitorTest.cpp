#include <gtest/gtest.h>
#include "monitor/ServiceMonitor.cpp"  // Include implementation directl
#include "monitor/ROSServiceDiscovery.cpp"  // Include implementation directl
#include "logger/Logger.cpp"

using namespace ros2::adsw_manager;
using namespace lifecycle_manager;

class ServiceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        ins = std::make_unique<ServiceMonitor>();
    }

    void TearDown() override {
        ins.reset();
    }

    std::unique_ptr<ServiceMonitor> ins;
};

TEST_F(ServiceMonitorTest, AddAndGetService_TC001) {
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
