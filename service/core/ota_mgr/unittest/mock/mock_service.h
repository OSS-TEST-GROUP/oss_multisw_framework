#ifndef MOCK_SERVICE_H_
#define MOCK_SERVICE_H_

#include <gmock/gmock.h>
#include "otam_main/otam_main.h" // Includes the Service base class definition

// Mock for the generic Service interface.
class MockService : public Service {
 public:
  MOCK_METHOD(void, Initialize, (), (override));
  MOCK_METHOD(void, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
};

// Mock for OtamDiagnosticService to verify error reporting.
class MockDiagnosticService : public Service {
 public:
  MOCK_METHOD(void, Initialize, (), (override));
  MOCK_METHOD(void, Start, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(void, UpdateErrorStatus, (const std::string&), ());
};

#endif  // MOCK_SERVICE_H_