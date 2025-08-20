#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define protected public
#define private public 
#include "ServiceMonitor.hpp"
#undef private
#undef protected

#include "common/AppUtils.hpp"
#include "lifecycle/LifecycleDiagReport.hpp"
#include "mock/MockServiceRegistry.hpp"
#include "mock/MockMessageTransmitter.hpp"
#include "mock/MockAppStateReader.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>

using namespace adsw_manager::agl;
using namespace testing;
using namespace lifecyclemgr;
using namespace eprosima::fastdds::dds;

class ServiceMonitorTest : public ::testing::Test {
    protected:
        std::shared_ptr<MockServiceRegistry> registry;
        std::shared_ptr<MockMessageTransmitter> transmitter;
        ServiceMonitor monitor;
        DomainParticipant* participant;

        void SetUp() {
            registry = std::make_shared<MockServiceRegistry>();
            transmitter = std::make_shared<MockMessageTransmitter>();
            DomainParticipantQos qos;
            participant = DomainParticipantFactory::get_instance()->create_participant(0, qos);

            ASSERT_TRUE(monitor.Initialize(participant, registry, transmitter));
            monitor.timeout_ = std::chrono::seconds(2);
        }

        MockAppStateReader* EmplaceReader(const std::string& app, std::chrono::steady_clock::time_point last) {
            auto r = std::make_unique<MockAppStateReader>(last);
            auto* raw = r.get();
            monitor.readers_[app] = std::move(r);
            return raw;
        }
};

// ----------------------------- Initialize -----------------------------

TEST_F(ServiceMonitorTest, Initialize_StoresDeps_AndReturnsTrue)
{
    ServiceMonitor mon;
    auto ok = mon.Initialize(nullptr, registry, transmitter);
    EXPECT_TRUE(ok);
    EXPECT_EQ(mon.participant_, nullptr); // 전달한 participant
    EXPECT_EQ(mon.service_registry_.get(), registry.get());
    EXPECT_EQ(mon.message_transmitter_.get(), transmitter.get());
}

// ----------------------------- StartMonitoring/StopMonitoring -----------------------------

TEST_F(ServiceMonitorTest, StartMonitoring_CreatesReadersFromRegistryList)
{
    // 레지스트리 목록 준비
    ServiceStatus a;
    a.name() = "appA";
    ServiceStatus b;
    b.name() = "appB";
    std::vector<ServiceStatus> list{a, b};
    EXPECT_CALL(*registry, GetAppList()).WillRepeatedly(Return(list));

    // 시작
    monitor.StartMonitoring();

    // 바로 멈춰도 readers_ 생성은 StartMonitoring 내부에서 끝남
    EXPECT_TRUE(monitor.running_); // running 플래그 ON
    EXPECT_EQ(monitor.readers_.count("appA"), 1u);
    EXPECT_EQ(monitor.readers_.count("appB"), 1u);

    monitor.StopMonitoring(); // 정리
}

TEST_F(ServiceMonitorTest, StopMonitoring_StopsThreadAndJoins)
{
    // 레지스트리 비어도 start/stop 동작에는 문제 없음
    std::vector<ServiceStatus> empty;
    EXPECT_CALL(*registry, GetAppList()).WillRepeatedly(Return(empty));

    monitor.StartMonitoring();
    // 바로 멈춤: 타이밍 민감도 낮게 유지 (루프 1초 sleep 전에 running_=false가 되면 바로 종료)
    monitor.StopMonitoring();

    EXPECT_FALSE(monitor.running_);
    EXPECT_FALSE(monitor.monitoring_thread_.joinable()); // join 완료되면 false
}

// ----------------------------- CheckAppState -----------------------------

TEST_F(ServiceMonitorTest, CheckAppState_Timeout_TransitionsToDetached_AndPublishes) {
    ServiceStatus info;
    info.name() = "appA";
    info.service_state() = ServiceState::RUNNING;
    std::vector<ServiceStatus> list{info};
    
    EXPECT_CALL(*registry, GetAppList()).WillRepeatedly(Return(list));

    auto now = std::chrono::steady_clock::now();
    EmplaceReader("appA", now - std::chrono::seconds(10)); // 타임아웃 훨씬 이전

    EXPECT_CALL(*registry, UpdateAppState("appA", ServiceState::NOT_RUNNING)).Times(1);
    EXPECT_CALL(*transmitter, PublishAppState()).Times(1);

    monitor.CheckAppState();
}