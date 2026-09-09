// F004/F005: ActorService unit tests
// Tests ProcessPendingUpdate vehicle conditions (F004)
// and Lifecycle Manager OTA mode flow (F005)
//
// Uses SetUpTestSuite/TearDownTestSuite (one ActorService instance per suite)
// to avoid FastDDS callback races between test cases.
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "otam_actor_service/otam_actor_service.h"
#include "mock/mock_otam_server_service.h"
#include "mock/mock_otam_vehicle_service.h"
#include "mock/mock_otam_diagnostic_service.h"
#include "mock/mock_otam_client_service.h"

// lifecycle headers for SetServiceModeResponse
#include "lifecycle/SetServiceModeRequest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::AnyNumber;

// Use NiceMock to suppress unexpected-call warnings from background FastDDS threads.
// Background DDS listeners can call mock methods outside of test expectations;
// NiceMock prevents those calls from aborting the process.
using NiceMockServer  = ::testing::NiceMock<MockOtamServerService>;
using NiceMockVehicle = ::testing::NiceMock<MockOtamVehicleService>;
using NiceMockDiag    = ::testing::NiceMock<MockOtamDiagnosticService>;
using NiceMockClient  = ::testing::NiceMock<MockOtamClientService>;

class OtamActorServiceTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        actor_service_ = std::make_unique<OtamActorService>();
        actor_service_->Initialize();

        mock_server_  = new NiceMockServer();
        mock_vehicle_ = new NiceMockVehicle();
        mock_diag_    = new NiceMockDiag();
        mock_client_  = new NiceMockClient();

        actor_service_->SetOtamServerService(mock_server_);
        actor_service_->SetOtamVehicleService(mock_vehicle_);
        actor_service_->SetOtamDiagnosticService(mock_diag_);
        actor_service_->SetOtamClientService(mock_client_);
    }

    static void TearDownTestSuite() {
        actor_service_->Stop();
        // Let FastDDS callback threads drain before destroying the service object
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        actor_service_.reset();
        delete mock_server_;  mock_server_ = nullptr;
        delete mock_vehicle_; mock_vehicle_ = nullptr;
        delete mock_diag_;    mock_diag_ = nullptr;
        delete mock_client_;  mock_client_ = nullptr;
    }

    void SetUp() override {
        ::testing::Mock::VerifyAndClearExpectations(mock_server_);
        ::testing::Mock::VerifyAndClearExpectations(mock_vehicle_);
        ::testing::Mock::VerifyAndClearExpectations(mock_diag_);
        ::testing::Mock::VerifyAndClearExpectations(mock_client_);
        // Suppress unexpected diagnostic calls by default
        EXPECT_CALL(*mock_diag_, PublishOtaMgrInfo(_, _)).Times(AnyNumber());
        // Default: lifecycle enabled (existing behaviour). Tests override when needed.
        ON_CALL(*mock_server_, IsLifecycleEnabled()).WillByDefault(Return(true));
        ON_CALL(*mock_server_, GetBatteryThresholdPercent()).WillByDefault(Return(20.0));
        // Default timeout = 0 disables lifecycle watcher for tests that don't exercise it.
        // F011 tests override with a non-zero value.
        ON_CALL(*mock_server_, GetLifecycleTimeoutSec()).WillByDefault(Return(0));
        ON_CALL(*mock_server_, GetVissTimeoutSec()).WillByDefault(Return(0));
    }

    // Helper: deliver all three vehicle signals so ProcessPendingUpdate can run
    void DeliverVehicleInfo(int32_t req_id,
                            const std::string& state,
                            const std::string& speed,
                            const std::string& soc = "50") {
        actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.LowVoltageSystemState", state);
        actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.Speed", speed);
        actor_service_->OnVehicleInfoReceived(
            req_id, "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current", soc);
    }

    // Helper: trigger OnAppUpdateRequested, returns request_id used internally
    int32_t CreatePendingRequest(const std::string& app_name = "test_app",
                                  int32_t msg_id = 1) {
        otamanager::SetAppUpdate msg;
        msg.id(msg_id);
        msg.app_name(app_name);
        actor_service_->OnAppUpdateRequested(msg);
        return suite_request_id_++;
    }

    void SendServiceModeSuccess() {
        lifecyclemgr::SetServiceModeResponse response;
        response.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
        actor_service_->OnSetServiceModeResponseReceived(response);
    }

    void SendServiceModeFail() {
        lifecyclemgr::SetServiceModeResponse response;
        response.ret_code(lifecyclemgr::ReturnCode::FAILED);
        actor_service_->OnSetServiceModeResponseReceived(response);
    }

    static std::unique_ptr<OtamActorService> actor_service_;
    static NiceMockServer*   mock_server_;
    static NiceMockVehicle*  mock_vehicle_;
    static NiceMockDiag*     mock_diag_;
    static NiceMockClient*   mock_client_;
    static int32_t suite_request_id_;
};

std::unique_ptr<OtamActorService> OtamActorServiceTest::actor_service_;
NiceMockServer*   OtamActorServiceTest::mock_server_   = nullptr;
NiceMockVehicle*  OtamActorServiceTest::mock_vehicle_  = nullptr;
NiceMockDiag*     OtamActorServiceTest::mock_diag_     = nullptr;
NiceMockClient*   OtamActorServiceTest::mock_client_   = nullptr;
int32_t OtamActorServiceTest::suite_request_id_ = 1000;

// ========== F004: ProcessPendingUpdate vehicle condition tests ==========

// vehicle_state=acc → RunOtaUpdate이 호출됨
TEST_F(OtamActorServiceTest, F004_VehicleStateAcc_RunOtaUpdateCalled) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("app_acc"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("app_acc", 101);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "30");
}

// vehicle_state=ignition && speed=0 → RunOtaUpdate이 호출됨
TEST_F(OtamActorServiceTest, F004_VehicleStateIgnitionSpeedZero_RunOtaUpdateCalled) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("app_ignition"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("app_ignition", 102);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "ignition", "0");
}

// vehicle_state=drive, speed=50 → 조건 불충족, RunOtaUpdate 미호출
TEST_F(OtamActorServiceTest, F004_VehicleConditionNotMet_RunOtaUpdateNotCalled) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    int32_t req_id = CreatePendingRequest("app_drive", 103);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "drive", "50");
}

// vehicle_info_enabled=false → 차량 조건 건너뛰고 RunOtaUpdate 직접 호출
TEST_F(OtamActorServiceTest, F004_VehicleInfoDisabled_RunOtaUpdateCalledDirectly) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("app_noveh"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    CreatePendingRequest("app_noveh", 104);
    SendServiceModeSuccess();
}

// ========== F005: Lifecycle Manager OTA mode flow tests ==========

// SetServiceModeResponse SUCCESS + vehicle_info_enabled=true → RequestVehicleInfo 호출
TEST_F(OtamActorServiceTest, F005_ServiceModeSuccess_VehicleInfoEnabled_RequestsCalled) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, "Vehicle.LowVoltageSystemState")).Times(1);
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, "Vehicle.Speed")).Times(1);
    EXPECT_CALL(*mock_vehicle_,
                RequestVehicleInfo(_, "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current"))
        .Times(1);
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(AnyNumber());

    int32_t req_id = CreatePendingRequest("app_veh_check", 105);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0");
}

// SetServiceModeResponse SUCCESS + vehicle_info_enabled=false → RequestVehicleInfo 미호출
TEST_F(OtamActorServiceTest, F005_ServiceModeSuccess_VehicleInfoDisabled_SkipsToUpdate) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("app_noveh2"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    CreatePendingRequest("app_noveh2", 106);
    SendServiceModeSuccess();
}

// SetServiceModeResponse FAILED → RunOtaUpdate 미호출
TEST_F(OtamActorServiceTest, F005_ServiceModeFail_RunOtaUpdateNotCalled) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).Times(AnyNumber());
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    CreatePendingRequest("app_fail", 107);
    SendServiceModeFail();
}

// OtamServerService 미설정 시 OnAppUpdateRequested가 FAIL 응답으로 종료됨
TEST_F(OtamActorServiceTest, F005_NoServerService_AppUpdateFails) {
    actor_service_->SetOtamServerService(nullptr);

    otamanager::SetAppUpdate msg;
    msg.id(199);
    msg.app_name("app_no_server");

    EXPECT_NO_THROW(actor_service_->OnAppUpdateRequested(msg));

    // Restore for subsequent tests
    actor_service_->SetOtamServerService(mock_server_);
}

// ========== F006/F007: Integration tests (same fixture, same DDS lifecycle) ==========

// TC2: Full OTA success flow — vehicle_state=acc, vehicle_info_enabled=true
TEST_F(OtamActorServiceTest, F007_TC2_FullOtaFlow_VehicleConditionMet_ACC) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc2_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("tc2_app", 201);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "60");
}

// TC3: vehicle_info_enabled=false → skip vehicle check, RunOtaUpdate called directly
TEST_F(OtamActorServiceTest, F007_TC3_VehicleInfoDisabled_SkipsVehicleCheck) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc3_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    CreatePendingRequest("tc3_app", 202);
    SendServiceModeSuccess();
}

// TC4: Vehicle condition not met (drive/80) → RunOtaUpdate NOT called
TEST_F(OtamActorServiceTest, F007_TC4_VehicleConditionNotMet_UpdateAborted) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    int32_t req_id = CreatePendingRequest("tc4_app", 203);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "drive", "80");
}

// TC5: Ignition + speed=0 → RunOtaUpdate called
TEST_F(OtamActorServiceTest, F007_TC5_IgnitionSpeedZero_UpdateAllowed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc5_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("tc5_app", 204);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "ignition", "0");
}

// Lifecycle FAILED → RunOtaUpdate NOT triggered
TEST_F(OtamActorServiceTest, F007_LifecycleFail_UpdateNotStarted) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).Times(AnyNumber());
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    CreatePendingRequest("tc_fail_app", 205);
    SendServiceModeFail();
}

// Sequential requests: two separate requests each resolved correctly
TEST_F(OtamActorServiceTest, F007_SequentialRequests_EachProcessedCorrectly) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("seq_app_a")).WillOnce(Return(ReturnValue::SUCCESS));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("seq_app_b")).WillOnce(Return(ReturnValue::SUCCESS));

    CreatePendingRequest("seq_app_a", 206);
    SendServiceModeSuccess();  // resolves seq_app_a

    CreatePendingRequest("seq_app_b", 207);
    SendServiceModeSuccess();  // resolves seq_app_b
}

// ========== Lifecycle toggle: IsLifecycleEnabled() config ==========

// lifecycle_enabled=false + vehicle_info_enabled=false
// → SetServiceModeRequest 생략, ProcessPendingUpdate 직접 실행 → RunOtaUpdate 호출
TEST_F(OtamActorServiceTest, LifecycleDisabled_NoVehicle_RunsUpdateDirectly) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("nolc_app")).WillOnce(Return(ReturnValue::SUCCESS));

    otamanager::SetAppUpdate msg;
    msg.id(300);
    msg.app_name("nolc_app");
    actor_service_->OnAppUpdateRequested(msg);
    suite_request_id_++;
    // No SendServiceModeSuccess() — the flow must self-complete when lifecycle disabled
}

// ========== F009: 클라이언트 FAILED 알림 전파 ==========

// 차량 조건 미충족으로 FAIL 응답이 발행되면 클라이언트에 "FAILED" 상태가 알림됨
TEST_F(OtamActorServiceTest, F009_VehicleFail_NotifiesClientFailed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f009_fail_app", "FAILED")).Times(1);

    int32_t req_id = CreatePendingRequest("f009_fail_app", 310);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "drive", "80");
}

// Lifecycle FAIL 시에도 클라이언트에 FAILED가 전달됨
TEST_F(OtamActorServiceTest, F009_LifecycleFail_NotifiesClientFailed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f009_lc_app", "FAILED")).Times(1);

    CreatePendingRequest("f009_lc_app", 311);
    SendServiceModeFail();
}

// SUCCESS 시에는 FAILED 알림이 발행되지 않음 (회귀 방지)
TEST_F(OtamActorServiceTest, F009_Success_DoesNotNotifyFailed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f009_ok_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus(_, "FAILED")).Times(0);
    // SUCCESS 후 actor 는 백그라운드 폴링 스레드에서 NotifyAppUpdateStatus(app, "Unknown"/
    // "Running" 등)을 호출한다. "FAILED" 에 EXPECT_CALL 을 걸면 NiceMock 이라도 이 메서드의
    // 모든 호출이 검사 대상이 되어, FAILED 이외 호출이 "Unexpected mock call" 실패가 된다.
    // FAILED 이외 상태는 허용해 폴링 스레드 노이즈를 흡수한다(회귀 의도 = FAILED 미발행).
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus(_, ::testing::Ne("FAILED")))
        .Times(::testing::AnyNumber());

    CreatePendingRequest("f009_ok_app", 312);
    SendServiceModeSuccess();
}

// lifecycle_enabled=false + vehicle_info_enabled=true
// → SetServiceModeRequest 생략하되 차량 정보는 여전히 요청
TEST_F(OtamActorServiceTest, LifecycleDisabled_VehicleEnabled_StillRequestsVehicleInfo) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, "Vehicle.LowVoltageSystemState")).Times(1);
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, "Vehicle.Speed")).Times(1);
    EXPECT_CALL(*mock_vehicle_,
                RequestVehicleInfo(_, "Vehicle.Powertrain.TractionBattery.StateOfCharge.Current"))
        .Times(1);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("nolc_veh_app")).WillOnce(Return(ReturnValue::SUCCESS));

    otamanager::SetAppUpdate msg;
    msg.id(301);
    msg.app_name("nolc_veh_app");
    actor_service_->OnAppUpdateRequested(msg);
    int32_t req_id = suite_request_id_++;
    DeliverVehicleInfo(req_id, "acc", "0");
}

// ========== F011: Lifecycle timeout + NORMAL 복원 ==========
// Uses the real background watcher thread (enabled by fastdds.ignore_local_endpoints
// which suppresses the same-participant loopback that previously made this flaky).

// Lifecycle 타임아웃 시 FAIL 응답 + NORMAL_MODE 요청 전송
TEST_F(OtamActorServiceTest, F011_LifecycleTimeout_FailsAndRestoresNormalMode) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetLifecycleTimeoutSec()).WillRepeatedly(Return(1));
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f011_to", "FAILED")).Times(1);

    int before = actor_service_->GetLifecycleRequestCount();
    CreatePendingRequest("f011_to", 501);
    // Wait for real-time watcher to fire (timeout=1s + poll slice + restore work)
    std::this_thread::sleep_for(std::chrono::milliseconds(1300));
    int after = actor_service_->GetLifecycleRequestCount();
    // 2 writes: initial OTA_MODE + NORMAL restore (triggered via PublishSetAppUpdateResponse FAIL)
    EXPECT_EQ(after - before, 2);
}

// 타임아웃 이전에 응답이 오면 watcher가 취소되어 NORMAL 복원 미발동
TEST_F(OtamActorServiceTest, F011_ResponseBeforeTimeout_NoRestore) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetLifecycleTimeoutSec()).WillRepeatedly(Return(2));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f011_fast")).WillOnce(Return(ReturnValue::SUCCESS));

    int before = actor_service_->GetLifecycleRequestCount();
    CreatePendingRequest("f011_fast", 502);
    SendServiceModeSuccess();  // arrives before 2s timeout → cancels watcher
    // Let watcher's next poll observe the cancellation and exit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int after = actor_service_->GetLifecycleRequestCount();
    // Only the initial OTA_MODE write — SUCCESS path doesn't trigger NORMAL restore
    EXPECT_EQ(after - before, 1);
}

// ========== F012: VISS 응답 타임아웃 ==========

// 전체 VISS signals 미수신 → 타임아웃 후 FAIL + 클라이언트 FAILED
TEST_F(OtamActorServiceTest, F012_VissTimeout_FailsUpdate) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetVissTimeoutSec()).WillRepeatedly(Return(1));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f012_viss_to", "FAILED")).Times(1);

    CreatePendingRequest("f012_viss_to", 601);
    SendServiceModeSuccess();  // triggers RequestVehicleInfo + starts VISS watcher
    std::this_thread::sleep_for(std::chrono::milliseconds(1300));
}

// 일부 신호만 도착 → 타임아웃 시 여전히 FAIL
TEST_F(OtamActorServiceTest, F012_PartialVissSignals_TimesOut) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetVissTimeoutSec()).WillRepeatedly(Return(1));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f012_partial", "FAILED")).Times(1);

    int32_t req_id = CreatePendingRequest("f012_partial", 602);
    SendServiceModeSuccess();
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.LowVoltageSystemState", "acc");
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.Speed", "0");
    std::this_thread::sleep_for(std::chrono::milliseconds(1300));
}

// 모든 signals 도착 → 타임아웃 미발동 (watcher 정상 종료)
TEST_F(OtamActorServiceTest, F012_AllSignalsBeforeTimeout_NoTimeout) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetVissTimeoutSec()).WillRepeatedly(Return(2));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f012_ok")).WillOnce(Return(ReturnValue::SUCCESS));
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f012_ok", "FAILED")).Times(0);

    int32_t req_id = CreatePendingRequest("f012_ok", 603);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// ========== F013: OTA 실패 시 Lifecycle NORMAL 복원 ==========

// 차량 조건 미충족으로 FAIL → NORMAL_MODE 복원 DDS write 발생
TEST_F(OtamActorServiceTest, F013_VehicleFail_RestoresNormalMode) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    int before = actor_service_->GetLifecycleRequestCount();
    int32_t req_id = CreatePendingRequest("f013_drive", 701);
    (void)req_id;
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "drive", "80");  // fails condition → FAIL
    int after = actor_service_->GetLifecycleRequestCount();
    // Expect 2 writes: initial OTA_MODE + NORMAL restore on FAIL
    EXPECT_EQ(after - before, 2);
}

// RunOtaUpdate가 FAIL 반환 → NORMAL_MODE 복원
TEST_F(OtamActorServiceTest, F013_RunOtaUpdateFail_RestoresNormalMode) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f013_argocd_fail"))
        .WillOnce(Return(ReturnValue::FAIL));

    int before = actor_service_->GetLifecycleRequestCount();
    CreatePendingRequest("f013_argocd_fail", 702);
    SendServiceModeSuccess();
    int after = actor_service_->GetLifecycleRequestCount();
    // Expect 2 writes: OTA_MODE + NORMAL restore (RunOtaUpdate failed)
    EXPECT_EQ(after - before, 2);
}

// lifecycle_enabled=false → FAIL 시에도 NORMAL 복원 생략
TEST_F(OtamActorServiceTest, F013_LifecycleDisabled_FailSkipsRestore) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f013_nolc"))
        .WillOnce(Return(ReturnValue::FAIL));

    int before = actor_service_->GetLifecycleRequestCount();
    otamanager::SetAppUpdate msg;
    msg.id(703);
    msg.app_name("f013_nolc");
    actor_service_->OnAppUpdateRequested(msg);
    suite_request_id_++;
    int after = actor_service_->GetLifecycleRequestCount();
    // Expect 0 writes: no OTA_MODE (lifecycle disabled) and no NORMAL restore
    EXPECT_EQ(after - before, 0);
}

// RunOtaUpdate 성공 → NORMAL 복원 미발동
TEST_F(OtamActorServiceTest, F013_Success_NoRestore) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f013_ok"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int before = actor_service_->GetLifecycleRequestCount();
    CreatePendingRequest("f013_ok", 704);
    SendServiceModeSuccess();
    int after = actor_service_->GetLifecycleRequestCount();
    EXPECT_EQ(after - before, 1);  // only OTA_MODE, no NORMAL restore on success
}

// ========== F010: Battery State-of-Charge threshold ==========

// SoC 25% (>= threshold 20%) → RunOtaUpdate 호출됨
TEST_F(OtamActorServiceTest, F010_BatterySocAboveThreshold_Allowed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetBatteryThresholdPercent()).WillRepeatedly(Return(20.0));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f010_ok")).WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("f010_ok", 401);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0", "25");
}

// SoC 15% (< threshold 20%) → RunOtaUpdate 미호출, 클라이언트 FAILED
TEST_F(OtamActorServiceTest, F010_BatterySocBelowThreshold_Rejected) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetBatteryThresholdPercent()).WillRepeatedly(Return(20.0));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f010_low", "FAILED")).Times(1);

    int32_t req_id = CreatePendingRequest("f010_low", 402);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0", "15");
}

// SoC unparseable(빈 문자열) → 불명으로 간주하여 거부
TEST_F(OtamActorServiceTest, F010_BatterySocUnknown_Rejected) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetBatteryThresholdPercent()).WillRepeatedly(Return(20.0));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    EXPECT_CALL(*mock_client_, NotifyAppUpdateStatus("f010_nan", "FAILED")).Times(1);

    int32_t req_id = CreatePendingRequest("f010_nan", 403);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0", "not-a-number");
}

// 사용자 정의 threshold 10% → SoC 15%는 통과
TEST_F(OtamActorServiceTest, F010_CustomThreshold_AffectsDecision) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetBatteryThresholdPercent()).WillRepeatedly(Return(10.0));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("f010_custom")).WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = CreatePendingRequest("f010_custom", 404);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0", "15");
}

// ========== QA 커버리지 보강: 미커버 요청 핸들러 (docs/qa) ==========
// server API 경로(GetOtaSettings/SetOtaSettings)는 비-virtual 이라 실 HTTP-API 를
// 호출하므로 유닛에서 다루지 않는다. 여기서는 client 경유 핸들러의 정상 경로와
// 각 핸들러의 "서비스 미연결" 조기 반환 분기를 검증한다 (모두 동작 관찰 = 크래시 없음).

// OnGetAppListRequested: 등록 클라이언트가 없으면 빈 응답을 구성해 발행한다.
TEST_F(OtamActorServiceTest, QA_OnGetAppList_EmptyClientList_NoThrow) {
    otamanager::GetAppList msg;
    msg.id(9001);
    EXPECT_NO_THROW(actor_service_->OnGetAppListRequested(msg));
}

// OnGetAppListRequested: client service 미연결 시 조기 반환.
TEST_F(OtamActorServiceTest, QA_OnGetAppList_NoClientService_EarlyReturn) {
    actor_service_->SetOtamClientService(nullptr);
    otamanager::GetAppList msg;
    msg.id(9002);
    EXPECT_NO_THROW(actor_service_->OnGetAppListRequested(msg));
    actor_service_->SetOtamClientService(mock_client_);
}

// OnDeleteClientRegistrationRequested: 미등록 client_id 는 실 DeleteClientById 가
// FAIL 을 반환하고 응답을 구성한다.
TEST_F(OtamActorServiceTest, QA_OnDeleteClientRegistration_UnknownClient_NoThrow) {
    otamanager::DeleteOtaClientRegistration msg;
    msg.id(9003);
    msg.client_id(424242);  // 등록된 적 없는 client
    EXPECT_NO_THROW(actor_service_->OnDeleteClientRegistrationRequested(msg));
}

// OnDeleteClientRegistrationRequested: client service 미연결 분기.
TEST_F(OtamActorServiceTest, QA_OnDeleteClientRegistration_NoClientService_NoThrow) {
    actor_service_->SetOtamClientService(nullptr);
    otamanager::DeleteOtaClientRegistration msg;
    msg.id(9004);
    msg.client_id(1);
    EXPECT_NO_THROW(actor_service_->OnDeleteClientRegistrationRequested(msg));
    actor_service_->SetOtamClientService(mock_client_);
}

// OnAppUpdateSettingsRequested: server 미연결 시 FAIL 응답 + 진단 발행.
TEST_F(OtamActorServiceTest, QA_OnAppUpdateSettingsRequested_NoServer_NoThrow) {
    actor_service_->SetOtamServerService(nullptr);
    otamanager::GetAppUpdateSettings msg;
    msg.id(9005);
    msg.app_name("qa_settings_app");
    EXPECT_NO_THROW(actor_service_->OnAppUpdateSettingsRequested(msg));
    actor_service_->SetOtamServerService(mock_server_);
}

// OnSetAppUpdateSettingsReceived: server 미연결 시 FAIL 응답 + 진단 발행.
TEST_F(OtamActorServiceTest, QA_OnSetAppUpdateSettingsReceived_NoServer_NoThrow) {
    actor_service_->SetOtamServerService(nullptr);
    otamanager::SetAppUpdateSettings msg;
    msg.id(9006);
    msg.app_name("qa_set_settings_app");
    msg.app_settings("{}");
    EXPECT_NO_THROW(actor_service_->OnSetAppUpdateSettingsReceived(msg));
    actor_service_->SetOtamServerService(mock_server_);
}

// OnAppUpdateStateRequested: server 미연결 시 FAIL 응답 + 진단 발행.
TEST_F(OtamActorServiceTest, QA_OnAppUpdateStateRequested_NoServer_NoThrow) {
    actor_service_->SetOtamServerService(nullptr);
    otamanager::GetAppUpdateState msg;
    msg.id(9007);
    msg.app_name("qa_state_app");
    EXPECT_NO_THROW(actor_service_->OnAppUpdateStateRequested(msg));
    actor_service_->SetOtamServerService(mock_server_);
}

// ===== server-available 경로 (server 메서드 virtual+mock) =====

// OnAppUpdateSettingsRequested: GetOtaSettings SUCCESS → 성공 응답 경로.
TEST_F(OtamActorServiceTest, QA_OnAppUpdateSettings_ServerSuccess) {
    AppSettingsResult r; r.ret_code = ReturnValue::SUCCESS; r.app_settings = "{\"prune\":true}";
    EXPECT_CALL(*mock_server_, GetOtaSettings("qa_ok")).WillOnce(Return(r));
    otamanager::GetAppUpdateSettings msg; msg.id(9101); msg.app_name("qa_ok");
    EXPECT_NO_THROW(actor_service_->OnAppUpdateSettingsRequested(msg));
}

// OnAppUpdateSettingsRequested: GetOtaSettings FAIL → 실패 응답 경로.
TEST_F(OtamActorServiceTest, QA_OnAppUpdateSettings_ServerFail) {
    AppSettingsResult r; r.ret_code = ReturnValue::FAIL;
    EXPECT_CALL(*mock_server_, GetOtaSettings(::testing::_)).WillOnce(Return(r));
    otamanager::GetAppUpdateSettings msg; msg.id(9102); msg.app_name("qa_fail");
    EXPECT_NO_THROW(actor_service_->OnAppUpdateSettingsRequested(msg));
}

// OnSetAppUpdateSettingsReceived: SetOtaSettings SUCCESS → 성공 응답 경로.
TEST_F(OtamActorServiceTest, QA_OnSetAppUpdateSettings_ServerSuccess) {
    EXPECT_CALL(*mock_server_, SetOtaSettings(::testing::_, ::testing::_))
        .WillOnce(Return(ReturnValue::SUCCESS));
    otamanager::SetAppUpdateSettings msg; msg.id(9103); msg.app_name("qa_set"); msg.app_settings("{}");
    EXPECT_NO_THROW(actor_service_->OnSetAppUpdateSettingsReceived(msg));
}

// OnAppUpdateStateRequested: GetOtaUpdateState 결과 처리 경로.
TEST_F(OtamActorServiceTest, QA_OnAppUpdateState_ServerResult) {
    AppUpdateStateResult r; r.ret_code = ReturnValue::SUCCESS;
    EXPECT_CALL(*mock_server_, GetOtaUpdateState(::testing::_)).WillOnce(Return(r));
    otamanager::GetAppUpdateState msg; msg.id(9104); msg.app_name("qa_state");
    EXPECT_NO_THROW(actor_service_->OnAppUpdateStateRequested(msg));
}

// ===== OTA 플로우 조건 브랜치 =====

// vehicle_info 비활성 → 차량 체크 건너뛰고 업데이트 진행.
TEST_F(OtamActorServiceTest, QA_VehicleInfoDisabled_DirectUpdate) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("qa_vdis")).WillOnce(Return(ReturnValue::SUCCESS));
    CreatePendingRequest("qa_vdis", 8001);
    SendServiceModeSuccess();
}

// vehicle_state=drive → 조건 미충족 → RunOtaUpdate 호출 안 됨(abort).
TEST_F(OtamActorServiceTest, QA_VehicleDrive_UpdateAborted) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    int32_t req_id = CreatePendingRequest("qa_drive", 8002);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "drive", "80");
}

// lifecycle 비활성 → service mode 요청 건너뜀.
TEST_F(OtamActorServiceTest, QA_LifecycleDisabled_SkipsServiceMode) {
    EXPECT_CALL(*mock_server_, IsLifecycleEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("qa_lcdis")).WillOnce(Return(ReturnValue::SUCCESS));
    CreatePendingRequest("qa_lcdis", 8003);
}

// 배터리 SoC < threshold → 거부(업데이트 안 함).
TEST_F(OtamActorServiceTest, QA_BatteryBelowThreshold_Rejected) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_server_, GetBatteryThresholdPercent()).WillRepeatedly(Return(50.0));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);
    int32_t req_id = CreatePendingRequest("qa_lowbat", 8004);
    SendServiceModeSuccess();
    DeliverVehicleInfo(req_id, "acc", "0", "10");  // soc 10 < 50
}

// OnVehicleInfoReceived: 미지의 request_id → no-op.
TEST_F(OtamActorServiceTest, QA_OnVehicleInfoReceived_UnknownId_NoThrow) {
    EXPECT_NO_THROW(actor_service_->OnVehicleInfoReceived(999999, "Vehicle.Speed", "0"));
}

// OnSetServiceModeResponseReceived: pending 없음 → no-op.
TEST_F(OtamActorServiceTest, QA_ServiceModeResponse_NoPending_NoThrow) {
    lifecyclemgr::SetServiceModeResponse r;
    r.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
    EXPECT_NO_THROW(actor_service_->OnSetServiceModeResponseReceived(r));
}

// SetOtamVehicleService: 콜백 등록(미커버 함수).
TEST_F(OtamActorServiceTest, QA_SetOtamVehicleService_NoThrow) {
    EXPECT_NO_THROW(actor_service_->SetOtamVehicleService(mock_vehicle_));
}
