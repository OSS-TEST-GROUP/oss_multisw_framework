#ifndef MOCK_OTAM_DIAGNOSTIC_SERVICE_H_
#define MOCK_OTAM_DIAGNOSTIC_SERVICE_H_

#include "otam_diagnostic_service/otam_diagnostic_service.h"
#include <gmock/gmock.h>

class MockOtamDiagnosticService : public OtamDiagnosticService {
public:
    MOCK_METHOD(void, PublishOtaMgrInfo,
                (int32_t err_code, const std::string& err_msg), (override));
    // F007 — heartbeat publish is virtual; mock so tests can assert (or
    // simply NiceMock-silence) the periodic call without a live DataWriter.
    MOCK_METHOD(void, PublishHeartbeatOnce, (), (override));
};

#endif  // MOCK_OTAM_DIAGNOSTIC_SERVICE_H_
