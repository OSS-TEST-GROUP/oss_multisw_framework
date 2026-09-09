// F003: VehicleService response callback unit tests
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <atomic>

#include "otam_vehicle_service/otam_vehicle_service.h"

using ::testing::_;

class OtamVehicleServiceTest : public ::testing::Test {
protected:
    OtamVehicleService vehicle_service_;
};

// RegisterCallback에 등록된 콜백이 OnVehicleInfoResponseReceived() 호출 시 실행됨
TEST_F(OtamVehicleServiceTest, RegisterCallback_CallbackIsInvoked) {
    bool called = false;
    int32_t recv_id = -1;
    std::string recv_name, recv_value;

    vehicle_service_.RegisterCallback(
        [&](int32_t id, const std::string& name, const std::string& value) {
            called = true;
            recv_id = id;
            recv_name = name;
            recv_value = value;
        });

    vehicle_service_.OnVehicleInfoResponseReceived(42, "Vehicle.Speed", "0");

    EXPECT_TRUE(called);
    EXPECT_EQ(recv_id, 42);
    EXPECT_EQ(recv_name, "Vehicle.Speed");
    EXPECT_EQ(recv_value, "0");
}

// 여러 콜백이 등록된 경우 모두 호출됨
TEST_F(OtamVehicleServiceTest, MultipleCallbacks_AllInvoked) {
    std::atomic<int> call_count{0};

    vehicle_service_.RegisterCallback(
        [&](int32_t, const std::string&, const std::string&) { ++call_count; });
    vehicle_service_.RegisterCallback(
        [&](int32_t, const std::string&, const std::string&) { ++call_count; });
    vehicle_service_.RegisterCallback(
        [&](int32_t, const std::string&, const std::string&) { ++call_count; });

    vehicle_service_.OnVehicleInfoResponseReceived(1, "Vehicle.LowVoltageSystemState", "acc");

    EXPECT_EQ(call_count.load(), 3);
}

// 콜백이 없어도 OnVehicleInfoResponseReceived()가 크래시 없이 동작함
TEST_F(OtamVehicleServiceTest, NoCallback_NocrashOnResponse) {
    EXPECT_NO_THROW(
        vehicle_service_.OnVehicleInfoResponseReceived(99, "Vehicle.Speed", "50"));
}

// 콜백에 다양한 vehicle_state 값이 정확히 전달됨
TEST_F(OtamVehicleServiceTest, VariousVehicleStates_CorrectlyPropagated) {
    std::vector<std::string> received_values;

    vehicle_service_.RegisterCallback(
        [&](int32_t, const std::string&, const std::string& value) {
            received_values.push_back(value);
        });

    vehicle_service_.OnVehicleInfoResponseReceived(1, "Vehicle.LowVoltageSystemState", "acc");
    vehicle_service_.OnVehicleInfoResponseReceived(2, "Vehicle.LowVoltageSystemState", "ignition");
    vehicle_service_.OnVehicleInfoResponseReceived(3, "Vehicle.LowVoltageSystemState", "drive");

    ASSERT_EQ(received_values.size(), 3u);
    EXPECT_EQ(received_values[0], "acc");
    EXPECT_EQ(received_values[1], "ignition");
    EXPECT_EQ(received_values[2], "drive");
}

// request_id가 콜백에 정확히 전달됨
TEST_F(OtamVehicleServiceTest, RequestId_CorrectlyPropagated) {
    std::vector<int32_t> received_ids;

    vehicle_service_.RegisterCallback(
        [&](int32_t id, const std::string&, const std::string&) {
            received_ids.push_back(id);
        });

    vehicle_service_.OnVehicleInfoResponseReceived(1000, "Vehicle.Speed", "0");
    vehicle_service_.OnVehicleInfoResponseReceived(1001, "Vehicle.Speed", "30");

    ASSERT_EQ(received_ids.size(), 2u);
    EXPECT_EQ(received_ids[0], 1000);
    EXPECT_EQ(received_ids[1], 1001);
}

// ===== QA 커버리지 보강: 순수 getter/setter (DDS 불필요) =====

// GetVehicleInfo: 응답으로 채워진 값을 이름으로 조회하면 반환한다.
TEST_F(OtamVehicleServiceTest, GetVehicleInfo_ReturnsStoredValue) {
    vehicle_service_.OnVehicleInfoResponseReceived(1, "Vehicle.Speed", "42");
    EXPECT_EQ(vehicle_service_.GetVehicleInfo("Vehicle.Speed"), "42");
}

// GetVehicleInfo: 저장된 적 없는 이름은 빈 문자열을 반환한다.
TEST_F(OtamVehicleServiceTest, GetVehicleInfo_UnknownName_ReturnsEmpty) {
    EXPECT_EQ(vehicle_service_.GetVehicleInfo("Vehicle.DoesNotExist"), "");
}

// GetVehicleInfo: 값이 비면 저장하지 않는다(빈 반환).
TEST_F(OtamVehicleServiceTest, GetVehicleInfo_EmptyValueNotStored) {
    vehicle_service_.OnVehicleInfoResponseReceived(2, "Vehicle.Empty", "");
    EXPECT_EQ(vehicle_service_.GetVehicleInfo("Vehicle.Empty"), "");
}

// SetOtamDiagnosticService: nullptr 설정도 크래시 없이 동작한다(setter 커버).
TEST_F(OtamVehicleServiceTest, SetOtamDiagnosticService_NoThrow) {
    EXPECT_NO_THROW(vehicle_service_.SetOtamDiagnosticService(nullptr));
}
