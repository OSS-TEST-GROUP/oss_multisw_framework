#ifndef MOCK_OTAM_SERVER_SERVICE_H_
#define MOCK_OTAM_SERVER_SERVICE_H_

#include "otam_server_service/otam_server_service.h"
#include <gmock/gmock.h>

class MockOtamServerService : public OtamServerService {
public:
    MOCK_METHOD(bool, IsVehicleInfoEnabled, (), (const, override));
    MOCK_METHOD(bool, IsLifecycleEnabled, (), (const, override));
    MOCK_METHOD(double, GetBatteryThresholdPercent, (), (const, override));
    MOCK_METHOD(int, GetLifecycleTimeoutSec, (), (const, override));
    MOCK_METHOD(int, GetVissTimeoutSec, (), (const, override));
    MOCK_METHOD(ReturnValue, RunOtaUpdate, (const std::string& app_name), (override));
};

#endif  // MOCK_OTAM_SERVER_SERVICE_H_
