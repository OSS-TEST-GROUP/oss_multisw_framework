// ServiceRegistry::UpdateServiceList — adsw_manager 가 보내오는 서비스/컨테이너
// 목록을 레지스트리에 반영하는 경로.
//
// 왜 별도 바이너리인가:
//   ServiceRegistryTest 는 mock/mock_service.hpp 를 링크 심으로 쓴다. 그
//   헤더는 Service 의 메서드를 전역 Mock_Service 로 넘기는데, Service::GetName()
//   은 virtual 이 아니라 링크 심으로만 갈아끼울 수 있다. 그래서 그 바이너리
//   안에서는 실제 Service 객체를 만들 수 없다 — UpdateServiceList 는 목록에
//   없는 서비스를 std::make_shared<Service>(...) 로 직접 만들고 곧바로
//   GetName()/Print() 를 부르므로 Mock_Service 가 널이면 터진다.
//
//   서비스마다 다른 이름을 돌려줘야 하는데 전역 mock 은 하나뿐이라, 심을 그대로
//   두고 우회하는 방법도 없다. 그래서 진짜 Service.cpp 를 링크하는 바이너리를
//   따로 둔다. 저장소가 이미 쓰는 방식이다 (ServiceTest 도 진짜 Service.cpp 를
//   링크한다).
//
// 왜 필요했나:
//   ServiceRegistry.cpp 의 미커버 67줄 중 60줄이 이 함수 하나였다. 유일한
//   호출자가 UpdateServiceListEndPoint(실 DDS 수신 경로)라 유닛 레인에서 한
//   번도 실행되지 않았다. 함수 자체는 순수 로직이라 요청 객체만 만들어 직접
//   부르면 된다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <vector>

#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/Container.hpp"

// 링크 심. UpdateServiceList 는 LifecycleFactory 를 통해 트랜스미터와
// StateManager 를 얻는다. 실물을 링크하면 DDS participant 가 뜬다.
#include "unittest/mock/mock_lifecycle_factory.hpp"
#include "unittest/mock/mock_dds_transmitter.hpp"
#include "unittest/mock/mock_state_manager.hpp"
#include "unittest/mock/mock_manager_base.hpp"
// StateManager 생성자가 LifecycleManager& 를 받는다. 심만 필요하다.
#include "unittest/mock/mock_lifecycle_manager.hpp"

#include "lifecycle/UpdateServiceListRequest.hpp"

using namespace lifecycle_manager;
using ::testing::_;
using ::testing::Return;
using ::testing::SizeIs;

// 싱글턴 접근자도 심으로 둔다 (ServiceRegistryTest 와 같은 방식).
LifecycleFactory* LifecycleFactory::GetInstance() {
    static LifecycleFactory inst;
    return &inst;
}

namespace {

lifecyclemgr::Service MakeWireService(const std::string& name,
                                      const std::string& container = "",
                                      bool enabled = true) {
    lifecyclemgr::Service s;
    s.name(name);
    s.service_type(lifecyclemgr::ServiceType::NORMAL);
    s.recovery_policy(lifecyclemgr::RecoveryPolicy::ON_FAILURE);
    s.is_enabled(enabled);
    s.mw_type(lifecyclemgr::MwType::ROS2);
    s.container_name(container);
    return s;
}

lifecyclemgr::Container MakeWireContainer(const std::string& name,
                                          const std::string& image = "img:1") {
    lifecyclemgr::Container c;
    c.name(name);
    c.image(image);
    c.deploy_file("/etc/mmf/compose.yaml");
    c.container_namespace("mmf");
    c.type(lifecyclemgr::ContainerType::DOCKER_COMPOSE);
    return c;
}

class UpdateServiceListTest : public ::testing::Test {
  protected:
    void SetUp() override {
        reg_ = std::make_unique<ServiceRegistry>();

        factory_     = std::make_unique<::testing::NiceMock<MockLifecycleFactory>>();
        transmitter_ = std::make_shared<::testing::NiceMock<MockDDSTransmitter>>();
        Mock_LifecycleFactory = factory_.get();
        Mock_DDSTransmitter   = transmitter_.get();

        ON_CALL(*factory_, GetTransmitter()).WillByDefault(Return(transmitter_));
        ON_CALL(*transmitter_, SendMessage(_, _)).WillByDefault(Return(true));
        // 기본은 StateManager 없음. 필요한 테스트만 UseStateManager() 로 켠다.
        ON_CALL(*factory_, GetStateMgr())
            .WillByDefault(Return(std::shared_ptr<StateManager>()));
    }

    void TearDown() override {
        Mock_StateManager     = nullptr;
        Mock_DDSTransmitter   = nullptr;
        Mock_LifecycleFactory = nullptr;
        reg_.reset();
    }

    ::testing::NiceMock<MockStateManager>& UseStateManager() {
        owner_         = std::make_unique<::testing::NiceMock<MockLifecycleManager>>();
        state_manager_ = std::make_shared<::testing::NiceMock<MockStateManager>>(*owner_);
        Mock_StateManager = state_manager_.get();
        ON_CALL(*factory_, GetStateMgr()).WillByDefault(Return(state_manager_));
        return *state_manager_;
    }

    std::unique_ptr<ServiceRegistry> reg_;
    std::unique_ptr<::testing::NiceMock<MockLifecycleFactory>> factory_;
    std::shared_ptr<::testing::NiceMock<MockDDSTransmitter>>   transmitter_;
    std::unique_ptr<::testing::NiceMock<MockLifecycleManager>> owner_;
    std::shared_ptr<::testing::NiceMock<MockStateManager>>     state_manager_;
};

} // namespace

// ── 등록과 응답 ─────────────────────────────────────────────────────────────

TEST_F(UpdateServiceListTest, RegistersNewServices) {
    lifecyclemgr::UpdateServiceListRequest req;
    req.service_list({MakeWireService("alpha"), MakeWireService("beta")});

    EXPECT_TRUE(reg_->UpdateServiceList(&req));

    EXPECT_EQ(reg_->GetServiceNum(), 2);
    ASSERT_NE(reg_->GetService("alpha"), nullptr);
    ASSERT_NE(reg_->GetService("beta"), nullptr);
    EXPECT_EQ(reg_->GetService("alpha")->GetName(), "alpha");
}

TEST_F(UpdateServiceListTest, PublishFailurePropagates) {
    ON_CALL(*transmitter_, SendMessage(_, _)).WillByDefault(Return(false));

    lifecyclemgr::UpdateServiceListRequest req;
    req.service_list({MakeWireService("alpha")});

    EXPECT_FALSE(reg_->UpdateServiceList(&req))
        << "응답 발행이 실패했는데 성공으로 보고했다";
    // 발행 실패와 무관하게 등록 자체는 끝나 있어야 한다.
    EXPECT_NE(reg_->GetService("alpha"), nullptr);
}

TEST_F(UpdateServiceListTest, EmptyListStillPublishesAndSkipsStateManager) {
    auto& sm = UseStateManager();
    EXPECT_CALL(sm, ProcessServiceHandling(_)).Times(0);

    lifecyclemgr::UpdateServiceListRequest req;   // 서비스 0개
    EXPECT_TRUE(reg_->UpdateServiceList(&req));
    EXPECT_EQ(reg_->GetServiceNum(), 0);
}

TEST_F(UpdateServiceListTest, NonEmptyListIsHandedToStateManager) {
    auto& sm = UseStateManager();
    EXPECT_CALL(sm, ProcessServiceHandling(SizeIs(2))).Times(1);

    lifecyclemgr::UpdateServiceListRequest req;
    req.service_list({MakeWireService("alpha"), MakeWireService("beta")});

    EXPECT_TRUE(reg_->UpdateServiceList(&req));
}

TEST_F(UpdateServiceListTest, MissingStateManagerIsNotFatal) {
    // GetStateMgr() 가 널을 주는 상태 (기본값) — 종료 중에 실제로 가능하다.
    lifecyclemgr::UpdateServiceListRequest req;
    req.service_list({MakeWireService("alpha")});

    EXPECT_NO_THROW(reg_->UpdateServiceList(&req));
    EXPECT_EQ(reg_->GetServiceNum(), 1);
}

// ── 그룹 재사용 (DESIGN-1) ──────────────────────────────────────────────────
//
// 갱신마다 새 그룹을 만들면 그룹이 무한히 늘고, StateMonitor 가 빈 그룹에도
// GetServiceState 를 반복 발송한다.

TEST_F(UpdateServiceListTest, SecondCallReusesTheExistingGroup) {
    lifecyclemgr::UpdateServiceListRequest first;
    first.service_list({MakeWireService("alpha")});
    ASSERT_TRUE(reg_->UpdateServiceList(&first));

    ASSERT_EQ(reg_->GetGroupList().size(), 1u);
    const std::string group = reg_->GetGroupList().front();

    // 이미 등록된 alpha 가 포함된 두 번째 갱신 — 새 그룹을 만들면 안 된다.
    lifecyclemgr::UpdateServiceListRequest second;
    second.service_list({MakeWireService("alpha"), MakeWireService("beta")});
    ASSERT_TRUE(reg_->UpdateServiceList(&second));

    EXPECT_EQ(reg_->GetGroupList().size(), 1u) << "갱신마다 그룹이 늘고 있다";
    EXPECT_EQ(reg_->GetGroupList().front(), group);
    EXPECT_EQ(reg_->GetServiceNum(), 2);
}

TEST_F(UpdateServiceListTest, FreshRegistryAllocatesADistinctGroup) {
    lifecyclemgr::UpdateServiceListRequest first;
    first.service_list({MakeWireService("alpha")});
    ASSERT_TRUE(reg_->UpdateServiceList(&first));
    ASSERT_EQ(reg_->GetGroupList().size(), 1u);

    // 겹치는 서비스가 하나도 없으면 재사용할 그룹이 없다.
    ServiceRegistry other;
    lifecyclemgr::UpdateServiceListRequest second;
    second.service_list({MakeWireService("gamma")});
    ASSERT_TRUE(other.UpdateServiceList(&second));

    ASSERT_EQ(other.GetGroupList().size(), 1u);
    EXPECT_NE(other.GetGroupList().front(), reg_->GetGroupList().front())
        << "newGroup() 이 같은 이름을 두 번 냈다";
}

TEST_F(UpdateServiceListTest, ReRegistrationUpdatesInPlace) {
    lifecyclemgr::UpdateServiceListRequest first;
    first.service_list({MakeWireService("alpha", "web", /*enabled=*/true)});
    ASSERT_TRUE(reg_->UpdateServiceList(&first));
    auto svc = reg_->GetService("alpha");
    ASSERT_NE(svc, nullptr);
    EXPECT_TRUE(svc->isEnabled());
    EXPECT_EQ(svc->GetContainerName(), "web");

    // adsw_manager 가 새 saved-service-list.yaml 로 재시작하면 바뀔 수 있는 값들.
    lifecyclemgr::UpdateServiceListRequest second;
    second.service_list({MakeWireService("alpha", "db", /*enabled=*/false)});
    ASSERT_TRUE(reg_->UpdateServiceList(&second));

    EXPECT_EQ(reg_->GetService("alpha"), svc) << "같은 이름인데 객체가 새로 생겼다";
    EXPECT_EQ(reg_->GetServiceNum(), 1);
    EXPECT_FALSE(svc->isEnabled()) << "is_enabled 갱신이 반영되지 않았다";
    EXPECT_EQ(svc->GetContainerName(), "db") << "컨테이너 재배치가 반영되지 않았다";
}

// ── 컨테이너 반영 (F013) ────────────────────────────────────────────────────

TEST_F(UpdateServiceListTest, RegistersContainers) {
    lifecyclemgr::UpdateServiceListRequest req;
    req.container_list({MakeWireContainer("web"), MakeWireContainer("db")});
    req.service_list({MakeWireService("alpha", "web")});

    ASSERT_TRUE(reg_->UpdateServiceList(&req));

    ASSERT_NE(reg_->GetContainer("web"), nullptr);
    ASSERT_NE(reg_->GetContainer("db"), nullptr);
    EXPECT_EQ(reg_->GetAllContainers().size(), 2u);
    EXPECT_EQ(reg_->GetServicesInContainer("web"),
              std::vector<std::string>{"alpha"});
}

TEST_F(UpdateServiceListTest, KnownContainerIsUpdatedNotDuplicated) {
    lifecyclemgr::UpdateServiceListRequest first;
    first.container_list({MakeWireContainer("web", "img:1")});
    ASSERT_TRUE(reg_->UpdateServiceList(&first));
    auto web = reg_->GetContainer("web");
    ASSERT_NE(web, nullptr);

    lifecyclemgr::UpdateServiceListRequest second;
    second.container_list({MakeWireContainer("web", "img:2")});
    ASSERT_TRUE(reg_->UpdateServiceList(&second));

    EXPECT_EQ(reg_->GetAllContainers().size(), 1u)
        << "같은 이름 컨테이너가 중복 등록됐다";
    EXPECT_EQ(reg_->GetContainer("web"), web) << "갱신이 아니라 새로 만들었다";
    EXPECT_EQ(web->GetImage(), "img:2");
}

TEST_F(UpdateServiceListTest, ContainerWithEmptyNameIsSkipped) {
    lifecyclemgr::UpdateServiceListRequest req;
    req.container_list({MakeWireContainer(""), MakeWireContainer("web")});

    ASSERT_TRUE(reg_->UpdateServiceList(&req));

    EXPECT_EQ(reg_->GetAllContainers().size(), 1u);
    EXPECT_NE(reg_->GetContainer("web"), nullptr);
}

// 이 저장소의 유닛 테스트는 gtest_main 을 링크하지 않고 각자 main 을 둔다.
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
