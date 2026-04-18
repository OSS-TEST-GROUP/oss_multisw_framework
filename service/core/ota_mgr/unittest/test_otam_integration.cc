// F006/F007: OTA Manager integration tests
// Tests the full service collaboration chain using real ActorService
// with mock dependencies injected at service boundaries.
//
// Uses SetUpTestSuite/TearDownTestSuite to avoid FastDDS callback races
// between test cases.
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

// lifecycle headers
#include "lifecycle/SetServiceModeRequest.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::AnyNumber;

// F006: Integration test fixture — one shared ActorService for the suite
class OtaManagerIntegrationFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        actor_service_ = std::make_unique<OtamActorService>();
        actor_service_->Initialize();

        mock_server_  = new MockOtamServerService();
        mock_vehicle_ = new MockOtamVehicleService();
        mock_diag_    = new MockOtamDiagnosticService();

        actor_service_->SetOtamServerService(mock_server_);
        actor_service_->SetOtamVehicleService(mock_vehicle_);
        actor_service_->SetOtamDiagnosticService(mock_diag_);
    }

    static void TearDownTestSuite() {
        actor_service_->Stop();
        // Let FastDDS callback threads drain before destroying the service object
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        actor_service_.reset();
        delete mock_server_;  mock_server_ = nullptr;
        delete mock_vehicle_; mock_vehicle_ = nullptr;
        delete mock_diag_;    mock_diag_ = nullptr;
    }

    void SetUp() override {
        ::testing::Mock::VerifyAndClearExpectations(mock_server_);
        ::testing::Mock::VerifyAndClearExpectations(mock_vehicle_);
        ::testing::Mock::VerifyAndClearExpectations(mock_diag_);
        EXPECT_CALL(*mock_diag_, PublishOtaMgrInfo(_, _)).Times(AnyNumber());
    }

    int32_t TriggerUpdateRequest(const std::string& app_name, int32_t msg_id) {
        OtaManager::SetAppUpdate msg;
        msg.id(msg_id);
        msg.app_name(app_name);
        actor_service_->OnAppUpdateRequested(msg);
        return suite_request_id_++;
    }

    void SimulateServiceModeSuccess() {
        lifecyclemgr::SetServiceModeResponse response;
        response.ret_code(lifecyclemgr::ReturnCode::SUCCESS);
        actor_service_->OnSetServiceModeResponseReceived(response);
    }

    void SimulateServiceModeFail() {
        lifecyclemgr::SetServiceModeResponse response;
        response.ret_code(lifecyclemgr::ReturnCode::FAILED);
        actor_service_->OnSetServiceModeResponseReceived(response);
    }

    static std::unique_ptr<OtamActorService>  actor_service_;
    static MockOtamServerService*   mock_server_;
    static MockOtamVehicleService*  mock_vehicle_;
    static MockOtamDiagnosticService* mock_diag_;
    static int32_t suite_request_id_;
};

std::unique_ptr<OtamActorService> OtaManagerIntegrationFixture::actor_service_;
MockOtamServerService*   OtaManagerIntegrationFixture::mock_server_   = nullptr;
MockOtamVehicleService*  OtaManagerIntegrationFixture::mock_vehicle_  = nullptr;
MockOtamDiagnosticService* OtaManagerIntegrationFixture::mock_diag_  = nullptr;
int32_t OtaManagerIntegrationFixture::suite_request_id_ = 2000;

// ========== F007 TC2: Full OTA success flow with vehicle_info_enabled=true ==========
// OnAppUpdateRequested → SendServiceModeRequest → OnSetServiceModeResponseReceived(SUCCESS)
// → RequestVehicleInfo x2 → OnVehicleInfoReceived x2 (acc) → RunOtaUpdate
TEST_F(OtaManagerIntegrationFixture, TC2_FullOtaFlow_VehicleConditionMet_ACC) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc2_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = TriggerUpdateRequest("tc2_app", 201);
    SimulateServiceModeSuccess();
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.LowVoltageSystemState", "acc");
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.Speed", "60");
}

// ========== F007 TC3: vehicle_info_enabled=false → skip vehicle check ==========
TEST_F(OtaManagerIntegrationFixture, TC3_VehicleInfoDisabled_SkipsVehicleCheck) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc3_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    TriggerUpdateRequest("tc3_app", 202);
    SimulateServiceModeSuccess();
}

// ========== F007 TC4: Vehicle condition not met → RunOtaUpdate not called ==========
TEST_F(OtaManagerIntegrationFixture, TC4_VehicleConditionNotMet_UpdateAborted) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    int32_t req_id = TriggerUpdateRequest("tc4_app", 203);
    SimulateServiceModeSuccess();
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.LowVoltageSystemState", "drive");
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.Speed", "80");
}

// ========== F007 TC5: Ignition + speed=0 also allowed ==========
TEST_F(OtaManagerIntegrationFixture, TC5_IgnitionSpeedZero_UpdateAllowed) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(AnyNumber());
    EXPECT_CALL(*mock_server_, RunOtaUpdate("tc5_app"))
        .WillOnce(Return(ReturnValue::SUCCESS));

    int32_t req_id = TriggerUpdateRequest("tc5_app", 204);
    SimulateServiceModeSuccess();
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.LowVoltageSystemState", "ignition");
    actor_service_->OnVehicleInfoReceived(req_id, "Vehicle.Speed", "0");
}

// ========== Lifecycle FAILED → RunOtaUpdate not triggered ==========
TEST_F(OtaManagerIntegrationFixture, LifecycleFail_UpdateNotStarted) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).Times(AnyNumber());
    EXPECT_CALL(*mock_vehicle_, RequestVehicleInfo(_, _)).Times(0);
    EXPECT_CALL(*mock_server_, RunOtaUpdate(_)).Times(0);

    TriggerUpdateRequest("tc_fail_app", 205);
    SimulateServiceModeFail();
}

// ========== Multiple sequential requests ==========
TEST_F(OtaManagerIntegrationFixture, SequentialRequests_EachProcessedCorrectly) {
    EXPECT_CALL(*mock_server_, IsVehicleInfoEnabled()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("seq_app_a")).WillOnce(Return(ReturnValue::SUCCESS));
    EXPECT_CALL(*mock_server_, RunOtaUpdate("seq_app_b")).WillOnce(Return(ReturnValue::SUCCESS));

    // First request
    TriggerUpdateRequest("seq_app_a", 206);
    SimulateServiceModeSuccess();  // resolves seq_app_a

    // Second request (after first is fully resolved)
    TriggerUpdateRequest("seq_app_b", 207);
    SimulateServiceModeSuccess();  // resolves seq_app_b
}
