#ifndef MOCK_OTAM_VEHICLE_SERVICE_H_
#define MOCK_OTAM_VEHICLE_SERVICE_H_

#include "otam_vehicle_service/otam_vehicle_service.h"
#include <gmock/gmock.h>

class MockOtamVehicleService : public OtamVehicleService {
public:
    MOCK_METHOD(void, RequestVehicleInfo,
                (int32_t request_id, const std::string& name), (override));
};

#endif  // MOCK_OTAM_VEHICLE_SERVICE_H_
