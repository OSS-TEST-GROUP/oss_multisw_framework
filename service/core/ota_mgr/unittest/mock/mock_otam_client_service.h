#ifndef MOCK_OTAM_CLIENT_SERVICE_H_
#define MOCK_OTAM_CLIENT_SERVICE_H_

#include "otam_client_service/otam_client_service.h"
#include <gmock/gmock.h>

class MockOtamClientService : public OtamClientService {
public:
    MOCK_METHOD(void, NotifyAppUpdateStatus,
                (const std::string& app_name, const std::string& status),
                (override));
};

#endif  // MOCK_OTAM_CLIENT_SERVICE_H_
