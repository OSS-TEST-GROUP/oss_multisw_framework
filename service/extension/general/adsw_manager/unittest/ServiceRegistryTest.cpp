// general adsw_manager — ServiceRegistry / Service 유닛테스트
//
// ServiceRegistry 는 이 모듈에서 가장 큰 파일(751줄)인데 테스트가 하나도 없었다.
// 내용은 크게 넷이다.
//   1) 서비스·그룹 인메모리 관리        — 순수 자료구조
//   2) YAML 서비스 목록 로드            — 파일 I/O, tmpdir 로 충분
//   3) ros2 launch/run 커맨드 추출      — 정규식, 순수 함수
//   4) Save / LoadServiceSaved 왕복     — 파일 I/O + enum 문자열 매핑
//
// 4번은 특히 회귀가 아팠던 곳이다. 예전에 정수 enum 을 쓰다가 LoadServiceSaved
// 의 문자열 비교가 전부 빗나가 조용히 기본값으로 떨어졌고, ctrl/SetRecoveryPolicy
// 한 번 왕복하면 ON_FAILURE 재시작 루프가 돌았다. 왕복 항등을 고정한다.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>
#include <memory>
#include <string>

#include "service/ServiceRegistry.hpp"
#include "service/Service.hpp"
#include "service/ServiceType.hpp"
#include "service/ServiceState.hpp"
#include "service/RecoveryPolicy.hpp"
#include "control/ContainerServiceController.hpp"

#include "mock/MockLogger.hpp"
#include "mock/MockADSWManager.hpp"

using ::testing::HasSubstr;
using general::adsw_manager::RecoveryPolicy;
using general::adsw_manager::Service;
using general::adsw_manager::ServiceRegistry;
using general::adsw_manager::ServiceState;
using general::adsw_manager::ServiceType;
using general::adsw_manager::ScopedMockADSWManager;
using general::adsw_manager::ContainerServiceController;

namespace {

// 테스트마다 격리된 디렉토리를 만들고 SAVED_REGISTRY_BACKUP 을 거기로 돌린다.
// ServiceRegistry 생성자가 이 환경변수를 읽어 저장 경로를 정한다.
class TempDir {
public:
    explicit TempDir(const std::string& tag) {
        path_ = "/tmp/general-registry-test-" + tag + "-" + std::to_string(::getpid());
        std::string cmd = "rm -rf '" + path_ + "' && mkdir -p '" + path_ + "'";
        [[maybe_unused]] int rc = std::system(cmd.c_str());
    }
    ~TempDir() {
        std::string cmd = "rm -rf '" + path_ + "'";
        [[maybe_unused]] int rc = std::system(cmd.c_str());
    }
    const std::string& path() const { return path_; }

    void Write(const std::string& rel, const std::string& body) const {
        std::ofstream f(path_ + "/" + rel);
        f << body;
    }
    bool Exists(const std::string& rel) const {
        std::ifstream f(path_ + "/" + rel);
        return f.good();
    }
private:
    std::string path_;
};

std::shared_ptr<Service> MakeService(const std::string& name,
                                     ServiceType type = ServiceType::NORMAL,
                                     RecoveryPolicy pol = RecoveryPolicy::ALWAYS,
                                     bool enabled = true) {
    return std::make_shared<Service>(name, type, pol, enabled);
}

class ServiceRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_ = std::make_unique<ScopedMockLogger>();
        mgr_    = std::make_unique<ScopedMockADSWManager>();
        tmp_    = std::make_unique<TempDir>("sr");
        ::setenv("SAVED_REGISTRY_BACKUP", tmp_->path().c_str(), 1);

        // LoadServiceSaved 는 GetServiceController()->GetContainer() 가 살아
        // 있어야 진행한다. 실물을 하나 만들어 물려 준다 (Initialize 가
        // ContainerController 를 생성한다).
        controller_ = std::make_shared<ContainerServiceController>();
        controller_->Initialize();
        ON_CALL(**mgr_, GetServiceController())
            .WillByDefault(::testing::Return(controller_));

        reg_ = std::make_unique<ServiceRegistry>();
    }
    void TearDown() override {
        reg_.reset();
        controller_.reset();
        ::unsetenv("SAVED_REGISTRY_BACKUP");
        tmp_.reset();
        mgr_.reset();
        logger_.reset();
    }

    std::unique_ptr<ScopedMockLogger>       logger_;
    std::unique_ptr<ScopedMockADSWManager>  mgr_;
    std::unique_ptr<TempDir>                tmp_;
    std::shared_ptr<ContainerServiceController> controller_;
    std::unique_ptr<ServiceRegistry>        reg_;
};

} // namespace

// ── 그룹 관리 ───────────────────────────────────────────────────────────────

TEST_F(ServiceRegistryTest, GroupAddRemoveAndQuery) {
    EXPECT_FALSE(reg_->IsGroup("g1"));
    reg_->AddGroup("g1");
    EXPECT_TRUE(reg_->IsGroup("g1"));
    EXPECT_EQ(reg_->GetGroupNum(), 1u);

    reg_->RemoveGroup("g1");
    EXPECT_FALSE(reg_->IsGroup("g1"));
    EXPECT_EQ(reg_->GetGroupNum(), 0u);
}

TEST_F(ServiceRegistryTest, AddGroupTwiceIsIdempotent) {
    reg_->AddGroup("g1");
    reg_->AddGroup("g1");
    EXPECT_EQ(reg_->GetGroupNum(), 1u);
}

TEST_F(ServiceRegistryTest, RemoveUnknownGroupIsSafe) {
    EXPECT_NO_THROW(reg_->RemoveGroup("nope"));
    EXPECT_EQ(reg_->GetGroupNum(), 0u);
}

// ── 서비스 관리 ─────────────────────────────────────────────────────────────

TEST_F(ServiceRegistryTest, AddAndGetService) {
    auto s = MakeService("svc1");
    EXPECT_TRUE(reg_->AddService(s, "g1"));
    EXPECT_EQ(reg_->GetService("svc1"), s);
    EXPECT_EQ(reg_->GetServicesNum(), 1u);
}

TEST_F(ServiceRegistryTest, GetUnknownServiceReturnsNull) {
    EXPECT_EQ(reg_->GetService("ghost"), nullptr);
}

TEST_F(ServiceRegistryTest, AddNullServiceRejected) {
    EXPECT_FALSE(reg_->AddService(nullptr, "g1"));
    EXPECT_EQ(reg_->GetServicesNum(), 0u);
}

TEST_F(ServiceRegistryTest, RemoveServiceDropsIt) {
    reg_->AddService(MakeService("svc1"), "g1");
    reg_->RemoveService("svc1");
    EXPECT_EQ(reg_->GetService("svc1"), nullptr);
    EXPECT_EQ(reg_->GetServicesNum(), 0u);
}

TEST_F(ServiceRegistryTest, RemoveUnknownServiceIsSafe) {
    EXPECT_NO_THROW(reg_->RemoveService("ghost"));
}

TEST_F(ServiceRegistryTest, GetServicesByGroupFiltersCorrectly) {
    reg_->AddService(MakeService("a"), "g1");
    reg_->AddService(MakeService("b"), "g1");
    reg_->AddService(MakeService("c"), "g2");

    EXPECT_EQ(reg_->GetServicesByGroup("g1").size(), 2u);
    EXPECT_EQ(reg_->GetServicesByGroup("g2").size(), 1u);
    EXPECT_TRUE(reg_->GetServicesByGroup("g3").empty());
}

TEST_F(ServiceRegistryTest, GetServiceNumInGroup) {
    reg_->AddService(MakeService("a"), "g1");
    reg_->AddService(MakeService("b"), "g1");
    EXPECT_EQ(reg_->GetServiceNumInGroup("g1"), 2u);
    EXPECT_EQ(reg_->GetServiceNumInGroup("none"), 0u);
}

TEST_F(ServiceRegistryTest, GetGroupNameByService) {
    auto s = MakeService("a");
    reg_->AddService(s, "mygroup");
    EXPECT_EQ(reg_->GetGroupNameByService(s), "mygroup");
}

TEST_F(ServiceRegistryTest, GetGroupNameByUnregisteredServiceIsEmpty) {
    EXPECT_TRUE(reg_->GetGroupNameByService(MakeService("loner")).empty());
}

TEST_F(ServiceRegistryTest, GetGroupNameByNullServiceIsSafe) {
    EXPECT_NO_THROW((void)reg_->GetGroupNameByService(nullptr));
}

// ── ros2 launch / run 커맨드 추출 (순수 정규식) ─────────────────────────────

TEST_F(ServiceRegistryTest, ExtractLaunchCommand_Single) {
    auto r = reg_->extractRos2LaunchCommands("ros2 launch my_pkg my_launch.py");
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(std::get<1>(r[0]), "my_pkg");
    EXPECT_EQ(std::get<2>(r[0]), "my_launch.py");
}

TEST_F(ServiceRegistryTest, ExtractLaunchCommand_Multiple) {
    auto r = reg_->extractRos2LaunchCommands(
        "ros2 launch pkg_a a.launch.py & ros2 launch pkg_b b.launch.py");
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(std::get<1>(r[0]), "pkg_a");
    EXPECT_EQ(std::get<1>(r[1]), "pkg_b");
}

TEST_F(ServiceRegistryTest, ExtractLaunchCommand_NoneWhenAbsent) {
    EXPECT_TRUE(reg_->extractRos2LaunchCommands("echo hello").empty());
    EXPECT_TRUE(reg_->extractRos2LaunchCommands("").empty());
}

TEST_F(ServiceRegistryTest, ExtractRunCommand_Single) {
    auto r = reg_->extractRos2RunCommands("ros2 run my_pkg my_node");
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(std::get<1>(r[0]), "my_pkg");
    EXPECT_EQ(std::get<2>(r[0]), "my_node");
}

TEST_F(ServiceRegistryTest, ExtractRunCommand_NoneWhenAbsent) {
    EXPECT_TRUE(reg_->extractRos2RunCommands("ros2 launch p f.py").empty());
}

// ── YAML 서비스 목록 로드 ───────────────────────────────────────────────────

TEST_F(ServiceRegistryTest, LoadServiceList_MissingFileFails) {
    EXPECT_FALSE(reg_->LoadServiceList(tmp_->path() + "/does-not-exist.yaml"));
}

TEST_F(ServiceRegistryTest, LoadServiceList_MalformedYamlFails) {
    tmp_->Write("bad.yaml", "spec: [unclosed\n");
    EXPECT_FALSE(reg_->LoadServiceList(tmp_->path() + "/bad.yaml"));
}

TEST_F(ServiceRegistryTest, LoadServiceList_ParsesLaunchCommandsIntoServices) {
    tmp_->Write("pod.yaml",
        "spec:\n"
        "  containers:\n"
        "    - name: app\n"
        "      command: [\"/bin/bash\", \"-c\"]\n"
        "      args:\n"
        "        - \"ros2 launch demo_pkg demo.launch.py\"\n");

    EXPECT_TRUE(reg_->LoadServiceList(tmp_->path() + "/pod.yaml"));
    EXPECT_TRUE(reg_->IsGroup("demo_pkg"));
    auto svc = reg_->GetService("demo_pkg");
    ASSERT_NE(svc, nullptr);
    EXPECT_EQ(svc->GetLaunchFile(), "demo.launch.py");
}

TEST_F(ServiceRegistryTest, LoadServiceList_NoArgsIsStillSuccess) {
    tmp_->Write("pod.yaml",
        "spec:\n"
        "  containers:\n"
        "    - name: app\n"
        "      command: [\"/bin/bash\"]\n");
    EXPECT_TRUE(reg_->LoadServiceList(tmp_->path() + "/pod.yaml"));
    EXPECT_EQ(reg_->GetServicesNum(), 0u);
}

// ── Save / LoadServiceSaved 왕복 ────────────────────────────────────────────
//
// 예전 회귀: 정수 enum 으로 저장하면 LoadServiceSaved 의 문자열 비교가 전부
// 빗나가 조용히 기본값으로 떨어졌다. 왕복이 항등인지 확인한다.

// yaml_path_ 는 no-arg LoadServiceList() 안에서만 정해진다. 그 전에 Save() 를
// 부르면 빈 경로로 열기에 실패한다 — 실사용 순서(로드 → 저장)를 그대로 따른다.
TEST_F(ServiceRegistryTest, SaveBeforeLoadFailsBecausePathIsUnset) {
    reg_->AddService(MakeService("svc1"), "g1");
    EXPECT_FALSE(reg_->Save());
}

TEST_F(ServiceRegistryTest, SaveWritesFile) {
    (void)reg_->LoadServiceList();   // yaml_path_ 확정 (원본 파일은 없어도 된다)
    reg_->AddService(MakeService("svc1"), "g1");
    EXPECT_TRUE(reg_->Save());
    EXPECT_TRUE(tmp_->Exists("saved-service-list.yaml"));
}

TEST_F(ServiceRegistryTest, SaveLoadRoundTripPreservesTypeAndPolicy) {
    (void)reg_->LoadServiceList();
    auto s = MakeService("svc1", ServiceType::ONESHOT, RecoveryPolicy::ON_FAILURE);
    s->SetPackage("pkg");
    s->SetLaunchFile("f.launch.py");
    reg_->AddService(s, "g1");
    ASSERT_TRUE(reg_->Save());

    ServiceRegistry fresh;
    ASSERT_TRUE(fresh.LoadServiceSaved(tmp_->path() + "/saved-service-list.yaml"));

    auto loaded = fresh.GetService("svc1");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetServiceType(), ServiceType::ONESHOT);
    EXPECT_EQ(loaded->GetRecoveryPolicy(), RecoveryPolicy::ON_FAILURE);
}

TEST_F(ServiceRegistryTest, SaveLoadRoundTripCoversEveryRecoveryPolicy) {
    (void)reg_->LoadServiceList();
    const RecoveryPolicy policies[] = {
        RecoveryPolicy::ALWAYS, RecoveryPolicy::ON_FAILURE,
        RecoveryPolicy::ON_SUCCESS, RecoveryPolicy::ON_NOT_RESPONDING,
        RecoveryPolicy::MANUAL,
    };
    int i = 0;
    for (auto p : policies) {
        reg_->AddService(MakeService("svc" + std::to_string(i++),
                                     ServiceType::NORMAL, p), "g1");
    }
    ASSERT_TRUE(reg_->Save());

    ServiceRegistry fresh;
    ASSERT_TRUE(fresh.LoadServiceSaved(tmp_->path() + "/saved-service-list.yaml"));

    i = 0;
    for (auto p : policies) {
        auto loaded = fresh.GetService("svc" + std::to_string(i++));
        ASSERT_NE(loaded, nullptr) << "svc" << (i - 1);
        EXPECT_EQ(loaded->GetRecoveryPolicy(), p) << "svc" << (i - 1);
    }
}

TEST_F(ServiceRegistryTest, LoadServiceSaved_MissingFileFails) {
    ServiceRegistry fresh;
    EXPECT_FALSE(fresh.LoadServiceSaved(tmp_->path() + "/nope.yaml"));
}

TEST_F(ServiceRegistryTest, LoadServiceSaved_MalformedFails) {
    tmp_->Write("broken.yaml", "services: [oops\n");
    ServiceRegistry fresh;
    EXPECT_FALSE(fresh.LoadServiceSaved(tmp_->path() + "/broken.yaml"));
}

TEST_F(ServiceRegistryTest, UpdateLastSentRecoveryIsSafeToCallRepeatedly) {
    reg_->AddService(MakeService("svc1"), "g1");
    EXPECT_NO_THROW(reg_->UpdateLastSentRecovery());
    EXPECT_NO_THROW(reg_->UpdateLastSentRecovery());
}

// ── ADSWManager 협력 경로 ───────────────────────────────────────────────────

TEST_F(ServiceRegistryTest, ProcessUpdateServiceListRequest_NoTransmitterIsSafe) {
    // 트랜스미터가 없으면 조용히 실패해야 한다 — 크래시 금지.
    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(::testing::Return(
            std::shared_ptr<general::adsw_manager::DDSTransmitter>()));
    EXPECT_NO_THROW((void)reg_->ProcessUpdateServiceListRequest());
}

TEST_F(ServiceRegistryTest, ProcessGetServiceStateRequest_NullMessageIsSafe) {
    EXPECT_NO_THROW((void)reg_->ProcessGetServiceStateRequest(nullptr));
}

// ═══════════════════════════════════════════════════════════════════════════
// Service 값 객체
// ═══════════════════════════════════════════════════════════════════════════

TEST(GeneralServiceTest, ConstructorSetsIdentityAndDefaults) {
    ScopedMockLogger lg;
    Service s("svc", ServiceType::ONESHOT, RecoveryPolicy::MANUAL, true);
    EXPECT_EQ(s.GetName(), "svc");
    EXPECT_EQ(s.GetServiceType(), ServiceType::ONESHOT);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::MANUAL);
    EXPECT_TRUE(s.IsEnabled());
    EXPECT_EQ(s.GetServiceState(), ServiceState::NOT_RUNNING);
    EXPECT_EQ(s.GetPID(), 0u);
    EXPECT_FALSE(s.IsDirectExec());
    EXPECT_FALSE(s.IsContainerWorkload());
}

TEST(GeneralServiceTest, StateAndPidMutators) {
    ScopedMockLogger lg;
    Service s("svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);

    s.SetServiceState(ServiceState::RUNNING);
    EXPECT_EQ(s.GetServiceState(), ServiceState::RUNNING);

    s.SetPID(4242);
    EXPECT_EQ(s.GetPID(), 4242u);

    s.SetExitCode(137);
    EXPECT_EQ(s.GetExitCode(), 137);

    s.SetRecoveryPolicy(RecoveryPolicy::ON_SUCCESS);
    EXPECT_EQ(s.GetRecoveryPolicy(), RecoveryPolicy::ON_SUCCESS);
}

TEST(GeneralServiceTest, RunningNodesTracksMax) {
    ScopedMockLogger lg;
    Service s("svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);

    s.SetRunningNodes(3);
    EXPECT_EQ(s.GetRunningNodes(), 3);
    EXPECT_EQ(s.GetMaxNodes(), 3);

    // 줄어들어도 최댓값은 유지된다.
    s.SetRunningNodes(1);
    EXPECT_EQ(s.GetRunningNodes(), 1);
    EXPECT_EQ(s.GetMaxNodes(), 3);

    // 최댓값을 넘어서면 갱신된다.
    s.SetRunningNodes(5);
    EXPECT_EQ(s.GetMaxNodes(), 5);
}

TEST(GeneralServiceTest, PackageAndLaunchFile) {
    ScopedMockLogger lg;
    Service s("svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);
    s.SetPackage("my_pkg");
    s.SetLaunchFile("my.launch.py");
    EXPECT_EQ(s.GetPackage(), "my_pkg");
    EXPECT_EQ(s.GetLaunchFile(), "my.launch.py");
}

TEST(GeneralServiceTest, ProcessMatchPatternAndFlags) {
    ScopedMockLogger lg;
    Service s("svc", ServiceType::NORMAL, RecoveryPolicy::ALWAYS, true);

    EXPECT_TRUE(s.GetProcessMatchPattern().empty());
    s.SetProcessMatchPattern("my-binary");
    EXPECT_EQ(s.GetProcessMatchPattern(), "my-binary");

    s.SetDirectExec(true);
    EXPECT_TRUE(s.IsDirectExec());

    s.SetContainerWorkload(true);
    EXPECT_TRUE(s.IsContainerWorkload());
}


// ── UpdateServiceList 메시지 구성 / Save 의 컨테이너 상세 ──────────────────
//
// 이 두 경로는 서비스뿐 아니라 "등록된 컨테이너"까지 있어야 본문이 돈다.
// 컨테이너가 없으면 루프를 그냥 지나쳐 절반이 미커버로 남는다.

namespace {
std::shared_ptr<general::adsw_manager::Container> MakeContainer(
        const std::string& name) {
    auto c = std::make_shared<general::adsw_manager::Container>(
        name, name + "-img", "/tmp/" + name + ".yaml", name + "-ns",
        general::adsw_manager::ContainerType::DOCKER);
    c->SetExtraArgs({"--network", "host"});
    c->SetCommandArgs({"/opt/run.sh"});
    return c;
}
} // namespace

TEST_F(ServiceRegistryTest, ProcessUpdateServiceListRequest_BuildsServiceAndContainerList) {
    reg_->AddService(MakeService("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS), "cnt");
    reg_->AddService(MakeService("svc2", ServiceType::ONESHOT, RecoveryPolicy::MANUAL), "cnt");
    controller_->GetContainer()->AddContainer(MakeContainer("cnt"));

    // 트랜스미터는 null 이라 마지막 전송에서 막히지만, 그 전까지 서비스·컨테이너
    // 목록을 채우는 본문은 전부 지난다.
    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(::testing::Return(
            std::shared_ptr<general::adsw_manager::DDSTransmitter>()));
    EXPECT_NO_THROW((void)reg_->ProcessUpdateServiceListRequest());
}

TEST_F(ServiceRegistryTest, ProcessUpdateServiceListRequest_NoControllerStillBuildsServices) {
    reg_->AddService(MakeService("svc1"), "cnt");
    ON_CALL(**mgr_, GetServiceController())
        .WillByDefault(::testing::Return(
            std::shared_ptr<ContainerServiceController>()));
    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(::testing::Return(
            std::shared_ptr<general::adsw_manager::DDSTransmitter>()));
    EXPECT_NO_THROW((void)reg_->ProcessUpdateServiceListRequest());
}

TEST_F(ServiceRegistryTest, SavePersistsContainerImageTypeAndArgs) {
    (void)reg_->LoadServiceList();
    auto svc = MakeService("svc1");
    svc->SetContainerWorkload(true);
    reg_->AddService(svc, "cnt");
    controller_->GetContainer()->AddContainer(MakeContainer("cnt"));

    ASSERT_TRUE(reg_->Save());

    // 저장된 YAML 에 컨테이너 상세가 실려야 한다.
    std::ifstream f(tmp_->path() + "/saved-service-list.yaml");
    std::string body((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    EXPECT_NE(body.find("cnt-img"), std::string::npos);
    EXPECT_NE(body.find("docker"), std::string::npos);
    EXPECT_NE(body.find("extra_args"), std::string::npos);
    EXPECT_NE(body.find("command_args"), std::string::npos);
}

TEST_F(ServiceRegistryTest, SaveLoadRoundTripPreservesContainerArgs) {
    (void)reg_->LoadServiceList();
    auto svc = MakeService("svc1");
    svc->SetContainerWorkload(true);
    reg_->AddService(svc, "cnt");
    controller_->GetContainer()->AddContainer(MakeContainer("cnt"));
    ASSERT_TRUE(reg_->Save());

    ServiceRegistry fresh;
    ASSERT_TRUE(fresh.LoadServiceSaved(tmp_->path() + "/saved-service-list.yaml"));
    auto loaded = fresh.GetService("svc1");
    ASSERT_NE(loaded, nullptr);
    EXPECT_TRUE(loaded->IsContainerWorkload());
}

TEST_F(ServiceRegistryTest, UpdateLastSentRecoveryTracksPolicyChange) {
    auto svc = MakeService("svc1", ServiceType::NORMAL, RecoveryPolicy::ALWAYS);
    reg_->AddService(svc, "cnt");

    // 스냅샷을 찍고 정책을 바꾸면 "변경됨" 으로 판정되어야 한다. 판정 자체는
    // private 이라 직접 못 부르고, 재전송 경로가 그걸 쓴다.
    reg_->UpdateLastSentRecovery();
    svc->SetRecoveryPolicy(RecoveryPolicy::MANUAL);

    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(::testing::Return(
            std::shared_ptr<general::adsw_manager::DDSTransmitter>()));
    EXPECT_NO_THROW((void)reg_->ProcessUpdateServiceListRequest());
    EXPECT_NO_THROW(reg_->UpdateLastSentRecovery());
}

// ═══════════════════════════════════════════════════════════════════════════
// TypeConvert — enum 전수 왕복
//
// 변환기는 enum 하나가 늘어날 때 조용히 default 로 새는 게 전형적인 사고다.
// 모든 값을 돌려 왕복이 항등인지 확인한다.
// ═══════════════════════════════════════════════════════════════════════════

#include "service/TypeConvert.hpp"

namespace {
using general::adsw_manager::TypeConvert;

template <typename Idl, typename Local, typename ToLocal, typename ToIdl>
void ExpectRoundTrip(const std::vector<std::pair<Idl, Local>>& pairs,
                     ToLocal to_local, ToIdl to_idl) {
    for (const auto& p : pairs) {
        EXPECT_EQ(to_local(p.first), p.second);
        EXPECT_EQ(to_idl(p.second), p.first);
    }
}
} // namespace

TEST(GeneralTypeConvertTest, ServiceTypeRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ServiceType, ServiceType>(
        {{lifecyclemgr::ServiceType::NORMAL,  ServiceType::NORMAL},
         {lifecyclemgr::ServiceType::ONESHOT, ServiceType::ONESHOT}},
        [](lifecyclemgr::ServiceType v) { return TypeConvert::ToServiceType(v); },
        [](ServiceType v) { return TypeConvert::ToServiceType(v); });
}

TEST(GeneralTypeConvertTest, ServiceStateRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::ServiceState, ServiceState>(
        {{lifecyclemgr::ServiceState::NOT_RUNNING,         ServiceState::NOT_RUNNING},
         {lifecyclemgr::ServiceState::INITIALIZING,        ServiceState::INITIALIZING},
         {lifecyclemgr::ServiceState::RUNNING,             ServiceState::RUNNING},
         {lifecyclemgr::ServiceState::RUNNING_NO_RESPONSE, ServiceState::RUNNING_NO_RESPONSE},
         {lifecyclemgr::ServiceState::TERMINATING,         ServiceState::TERMINATING}},
        [](lifecyclemgr::ServiceState v) { return TypeConvert::ToServiceState(v); },
        [](ServiceState v) { return TypeConvert::ToServiceState(v); });
}

TEST(GeneralTypeConvertTest, RecoveryPolicyRoundTrip) {
    ExpectRoundTrip<lifecyclemgr::RecoveryPolicy, RecoveryPolicy>(
        {{lifecyclemgr::RecoveryPolicy::R_DEFAULT,         RecoveryPolicy::R_DEFAULT},
         {lifecyclemgr::RecoveryPolicy::ALWAYS,            RecoveryPolicy::ALWAYS},
         {lifecyclemgr::RecoveryPolicy::ON_FAILURE,        RecoveryPolicy::ON_FAILURE},
         {lifecyclemgr::RecoveryPolicy::ON_SUCCESS,        RecoveryPolicy::ON_SUCCESS},
         {lifecyclemgr::RecoveryPolicy::ON_NOT_RESPONDING, RecoveryPolicy::ON_NOT_RESPONDING},
         {lifecyclemgr::RecoveryPolicy::MANUAL,            RecoveryPolicy::MANUAL}},
        [](lifecyclemgr::RecoveryPolicy v) { return TypeConvert::ToRecoveryPolicy(v); },
        [](RecoveryPolicy v) { return TypeConvert::ToRecoveryPolicy(v); });
}

TEST(GeneralTypeConvertTest, MwTypeRoundTrip) {
    using general::adsw_manager::MwType;
    ExpectRoundTrip<lifecyclemgr::MwType, MwType>(
        {{lifecyclemgr::MwType::ROS2,       MwType::ROS2},
         {lifecyclemgr::MwType::AGL,        MwType::AGL},
         {lifecyclemgr::MwType::AUTOSAR,    MwType::AUTOSAR},
         {lifecyclemgr::MwType::MW_NATIVE,  MwType::MW_NATIVE},
         {lifecyclemgr::MwType::MW_UNKNOWN, MwType::MW_UNKNOWN}},
        [](lifecyclemgr::MwType v) { return TypeConvert::ToMwType(v); },
        [](MwType v) { return TypeConvert::ToMwType(v); });
}

TEST(GeneralTypeConvertTest, ContainerTypeRoundTrip) {
    using general::adsw_manager::ContainerType;
    // 로컬 enum 은 UNKNOWN/K3S/DOCKER/DOCKER_COMPOSE, IDL 은
    // CT_UNKNOWN/DOCKER/DOCKER_COMPOSE/K3S 로 순서가 다르다. 값이 아니라
    // 의미로 매핑되는지 본다.
    EXPECT_EQ(TypeConvert::ToContainerType(ContainerType::DOCKER),
              lifecyclemgr::ContainerType::DOCKER);
    EXPECT_EQ(TypeConvert::ToContainerType(ContainerType::DOCKER_COMPOSE),
              lifecyclemgr::ContainerType::DOCKER_COMPOSE);
    EXPECT_EQ(TypeConvert::ToContainerType(ContainerType::K3S),
              lifecyclemgr::ContainerType::K3S);
    EXPECT_EQ(TypeConvert::ToContainerType(ContainerType::UNKNOWN),
              lifecyclemgr::ContainerType::CT_UNKNOWN);
}

TEST(GeneralTypeConvertTest, MwTypeOutOfRangeIsUnknownNotRos2) {
    using general::adsw_manager::MwType;
    // "모른다" 가 "ROS2 라우팅 대상이다" 로 바뀌면 안 된다.
    EXPECT_EQ(TypeConvert::ToMwType(static_cast<lifecyclemgr::MwType>(99)),
              MwType::MW_UNKNOWN);
    EXPECT_EQ(TypeConvert::ToMwType(static_cast<MwType>(99)),
              lifecyclemgr::MwType::MW_UNKNOWN);
}

TEST(GeneralTypeConvertTest, OutOfRangeFallsBackToSafeDefault) {
    EXPECT_EQ(TypeConvert::ToServiceType(static_cast<lifecyclemgr::ServiceType>(99)),
              ServiceType::NORMAL);
    EXPECT_EQ(TypeConvert::ToServiceState(static_cast<lifecyclemgr::ServiceState>(99)),
              ServiceState::NOT_RUNNING);
    EXPECT_EQ(TypeConvert::ToRecoveryPolicy(static_cast<lifecyclemgr::RecoveryPolicy>(99)),
              RecoveryPolicy::R_DEFAULT);
}

// ============================================================================
// lifecycle_mgr 로 나가는 두 요청 경로
//
// 왜 미커버였나:
//   ServiceRegistry.cpp 의 미커버 104줄 중 71줄이 이 두 함수였다
//   (ProcessUpdateServiceListRequest 48, ProcessGetServiceStateRequest 23).
//   둘 다 ADSWManager 싱글턴을 통해 트랜스미터를 얻어 DDS 로 내보내는데,
//   기존 테스트는 트랜스미터를 물려주지 않아 함수 진입 전에 갈렸다.
//
//   실 DDS 는 필요 없다. 엔드포인트를 등록해 두면 Write 까지 도달하고,
//   Initialize 하지 않은 엔드포인트의 Write 는 false 를 돌려줄 뿐이다.
//   여기서 고정하는 것은 "무엇을 언제 보내고 언제 보내지 않는가" 다.
// ============================================================================

#include "transmitter/DDSTransmitter.hpp"
#include "transmitter/DDSMessageEndpoint.hpp"
#include "endpoint/MessageTypes.hpp"
#include "lifecycle/GetServiceStateRequest.hpp"

namespace {

using general::adsw_manager::DDSMessageEndpoint;
using general::adsw_manager::DDSTransmitter;
using general::adsw_manager::MessageType;

// DDSMessageEndpoint 는 추상 클래스다. 순수 가상 4개만 채우면 등록할 수 있다.
class StubEndpoint : public DDSMessageEndpoint {
public:
    explicit StubEndpoint(std::string topic) : topic_(std::move(topic)) {}
    std::string GetTopic() override { return topic_; }
    std::string GetTypeName() override { return "StubType"; }
    std::shared_ptr<TypeSupport> GetTypeSupport() override { return nullptr; }
    void OnDataAvailable() override {}
private:
    std::string topic_;
};

constexpr const char* kUpdateTopic = "mmf/lifecyclemgr/UpdateServiceListRequest";

// 기존 픽스처에 트랜스미터·토픽 배선만 얹는다.
class RegistryRequestTest : public ServiceRegistryTest {
protected:
    void SetUp() override {
        ServiceRegistryTest::SetUp();
        tx_ = std::make_shared<DDSTransmitter>();
        ON_CALL(**mgr_, GetTransmitter()).WillByDefault(::testing::Return(tx_));
        // 널 토픽을 주면 GetMessageEndpoint 가 std::string(nullptr) 을 만든다.
        // 항상 실제 문자열을 준다.
        ON_CALL(**mgr_, GetTopic(::testing::_))
            .WillByDefault(::testing::Return(kUpdateTopic));
    }
    void TearDown() override {
        tx_.reset();
        ServiceRegistryTest::TearDown();
    }

    void RegisterEndpoint() {
        ASSERT_TRUE(tx_->AddMessageEndpoint(std::make_shared<StubEndpoint>(kUpdateTopic)));
    }

    // 서비스 두 개가 든 목록을 읽혀 ready 상태로 만든다.
    void LoadTwoServices() {
        tmp_->Write("pod.yaml",
            "spec:\n"
            "  containers:\n"
            "    - name: app\n"
            "      command: [\"/bin/bash\", \"-c\"]\n"
            "      args:\n"
            "        - \"ros2 launch alpha_pkg alpha.launch.py\"\n"
            "    - name: app2\n"
            "      command: [\"/bin/bash\", \"-c\"]\n"
            "      args:\n"
            "        - \"ros2 launch beta_pkg beta.launch.py\"\n");
        ASSERT_TRUE(reg_->LoadServiceList(tmp_->path() + "/pod.yaml"));
    }

    std::shared_ptr<DDSTransmitter> tx_;
};

} // namespace

// ── ProcessUpdateServiceListRequest ─────────────────────────────────────────

TEST_F(RegistryRequestTest, UpdateRequest_RefusedBeforeServiceListIsReady) {
    // 목록을 읽기 전에는 보낼 내용이 없다. 빈 목록을 보내면 lifecycle_mgr 이
    // 서비스가 사라진 것으로 오해한다.
    RegisterEndpoint();
    EXPECT_FALSE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_SendsOnceServiceListIsReady) {
    LoadTwoServices();
    RegisterEndpoint();

    EXPECT_TRUE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_SkippedWhenRecoveryUnchanged) {
    LoadTwoServices();
    RegisterEndpoint();
    ASSERT_TRUE(reg_->ProcessUpdateServiceListRequest());

    // lifecycle_mgr 의 응답이 도착하면 ADSWManager 가 스냅샷을 찍는다.
    reg_->UpdateLastSentRecovery();

    // 이후로는 바뀐 게 없으니 조용하다 — 성공으로 보고하되 발송은 하지 않는다.
    EXPECT_TRUE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_ResumesWhenRecoveryPolicyChanges) {
    LoadTwoServices();
    RegisterEndpoint();
    ASSERT_TRUE(reg_->ProcessUpdateServiceListRequest());
    reg_->UpdateLastSentRecovery();

    auto svc = reg_->GetService("alpha_pkg");
    ASSERT_NE(svc, nullptr);
    svc->SetRecoveryPolicy(RecoveryPolicy::MANUAL);

    // 정책이 바뀌었으면 다시 보내야 한다.
    EXPECT_TRUE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_FailsWithoutTransmitter) {
    LoadTwoServices();
    ON_CALL(**mgr_, GetTransmitter())
        .WillByDefault(::testing::Return(std::shared_ptr<DDSTransmitter>()));

    EXPECT_FALSE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_FailsWhenEndpointIsNotRegistered) {
    LoadTwoServices();
    // 트랜스미터는 있지만 이 토픽으로 등록된 엔드포인트가 없다.
    EXPECT_FALSE(reg_->ProcessUpdateServiceListRequest());
}

TEST_F(RegistryRequestTest, UpdateRequest_CarriesKnownContainers) {
    LoadTwoServices();
    RegisterEndpoint();

    // F013: 컨테이너 목록도 함께 실려 lifecycle_mgr 의 containers_ 를 채운다.
    auto container_ctrl = controller_->GetContainer();
    ASSERT_NE(container_ctrl, nullptr);

    EXPECT_TRUE(reg_->ProcessUpdateServiceListRequest());
}

// ── ProcessGetServiceStateRequest ───────────────────────────────────────────

TEST_F(RegistryRequestTest, GetStateRequest_NullMessageFails) {
    EXPECT_FALSE(reg_->ProcessGetServiceStateRequest(nullptr));
}

TEST_F(RegistryRequestTest, GetStateRequest_AnswersForNativeMwType) {
    LoadTwoServices();

    lifecyclemgr::GetServiceStateRequest req;
    req.mw_type(lifecyclemgr::MwType::MW_NATIVE);
    req.service_list({"alpha_pkg", "beta_pkg"});
    req.is_all_service(false);

    // 엔드포인트가 없어 실제 발송은 실패하지만, 응답 조립까지는 밟는다.
    EXPECT_NO_THROW(reg_->ProcessGetServiceStateRequest(&req));
}

TEST_F(RegistryRequestTest, GetStateRequest_AlsoAnswersLegacyRos2) {
    // 구버전 lifecycle_mgr / 저장된 목록이 아직 ROS2 로 물어올 수 있다.
    LoadTwoServices();

    lifecyclemgr::GetServiceStateRequest req;
    req.mw_type(lifecyclemgr::MwType::ROS2);
    req.service_list({"alpha_pkg"});
    req.is_all_service(false);

    EXPECT_NO_THROW(reg_->ProcessGetServiceStateRequest(&req));
}

TEST_F(RegistryRequestTest, GetStateRequest_UnknownServiceIsSkippedNotFatal) {
    LoadTwoServices();

    lifecyclemgr::GetServiceStateRequest req;
    req.mw_type(lifecyclemgr::MwType::MW_NATIVE);
    req.service_list({"alpha_pkg", "does-not-exist"});
    req.is_all_service(false);

    EXPECT_NO_THROW(reg_->ProcessGetServiceStateRequest(&req));
}

TEST_F(RegistryRequestTest, GetStateRequest_ForeignMwTypeIsIgnoredButAcknowledged) {
    // 내 소관이 아니면 응답하지 않는다. 다만 조용히 사라지면 안 되므로
    // 성공으로 돌려주고 로그를 남긴다.
    lifecyclemgr::GetServiceStateRequest req;
    req.mw_type(lifecyclemgr::MwType::AGL);
    req.is_all_service(true);

    EXPECT_TRUE(reg_->ProcessGetServiceStateRequest(&req));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
