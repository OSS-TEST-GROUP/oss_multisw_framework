#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "otam_main.h"        // SUT (System Under Test)
#include "mock_service.h"     // Our separated mocks

#include <stdexcept> // For std::runtime_error

// Test fixture for OtamMain
class OtamMainTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockService> mock_service_1;
  std::shared_ptr<MockService> mock_service_2;
  std::shared_ptr<MockDiagnosticService> mock_diagnostic_service;
  std::vector<std::shared_ptr<Service>> mock_services_vector;

  void SetUp() override {
    mock_service_1 = std::make_shared<MockService>();
    mock_service_2 = std::make_shared<MockService>();
    mock_diagnostic_service = std::make_shared<MockDiagnosticService>();

    mock_services_vector.push_back(mock_service_1);
    mock_services_vector.push_back(mock_service_2);
    mock_services_vector.push_back(mock_diagnostic_service); // Add diagnostic service to the list
  }
};

// Test case for Initialize method
TEST_F(OtamMainTest, InitializeCallsInitializeOnAllUnits) {
  OtamMain otam_main(mock_services_vector);

  // Expect Initialize to be called on all mock services
  EXPECT_CALL(*mock_service_1, Initialize()).Times(1);
  EXPECT_CALL(*mock_service_2, Initialize()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Initialize()).Times(1);

  otam_main.Initialize();
}

// Test case for Start method
TEST_F(OtamMainTest, StartCallsStartOnAllUnits) {
  OtamMain otam_main(mock_services_vector);

  // Expect Start to be called on all mock services
  EXPECT_CALL(*mock_service_1, Start()).Times(1);
  EXPECT_CALL(*mock_service_2, Start()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Start()).Times(1);

  otam_main.Start();
}

// Test case for Stop method
TEST_F(OtamMainTest, StopCallsStopOnAllUnits) {
  OtamMain otam_main(mock_services_vector);

  // Expect Stop to be called on all mock services
  EXPECT_CALL(*mock_service_1, Stop()).Times(1);
  EXPECT_CALL(*mock_service_2, Stop()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Stop()).Times(1);

  otam_main.Stop();
}

// Test case for error handling during initialization
TEST_F(OtamMainTest, InitializeHandlesErrorAndReportsToDiagnosticService) {
  // Simulate an error during initialization of mock_service_1
  EXPECT_CALL(*mock_service_1, Initialize())
      .WillOnce(::testing::Throw(std::runtime_error("Simulated Init Error")));
  EXPECT_CALL(*mock_service_2, Initialize()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Initialize()).Times(1);
  // Expect UpdateErrorStatus to be called on the diagnostic service
  EXPECT_CALL(*mock_diagnostic_service, UpdateErrorStatus(::testing::HasSubstr("Simulated Init Error")))
      .Times(1);

  OtamMain otam_main(mock_services_vector);
  otam_main.Initialize();
}

// Test case for error handling during start
TEST_F(OtamMainTest, StartHandlesErrorAndReportsToDiagnosticService) {
  // Ensure initialization passes before testing start errors
  EXPECT_CALL(*mock_service_1, Initialize()).Times(1);
  EXPECT_CALL(*mock_service_2, Initialize()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Initialize()).Times(1);
  
  // Simulate an error during start of mock_service_2
  EXPECT_CALL(*mock_service_1, Start()).Times(1);
  EXPECT_CALL(*mock_service_2, Start())
      .WillOnce(::testing::Throw(std::runtime_error("Simulated Start Error")));
  EXPECT_CALL(*mock_diagnostic_service, Start()).Times(1);
  // Expect UpdateErrorStatus to be called on the diagnostic service
  EXPECT_CALL(*mock_diagnostic_service, UpdateErrorStatus(::testing::HasSubstr("Simulated Start Error")))
      .Times(1);

  OtamMain otam_main(mock_services_vector);
  otam_main.Initialize(); // Necessary for Start to be called on all services
  otam_main.Start();
}

// Test case for error handling during stop (should not report to diagnostic service for stop)
TEST_F(OtamMainTest, StopHandlesErrorButDoesNotReportToDiagnosticService) {
  // Ensure initialization and start pass before testing stop errors
  EXPECT_CALL(*mock_service_1, Initialize()).Times(1);
  EXPECT_CALL(*mock_service_2, Initialize()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Initialize()).Times(1);
  EXPECT_CALL(*mock_service_1, Start()).Times(1);
  EXPECT_CALL(*mock_service_2, Start()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Start()).Times(1);

  // Simulate an error during stop of mock_service_1
  EXPECT_CALL(*mock_service_1, Stop())
      .WillOnce(::testing::Throw(std::runtime_error("Simulated Stop Error")));
  EXPECT_CALL(*mock_service_2, Stop()).Times(1);
  EXPECT_CALL(*mock_diagnostic_service, Stop()).Times(1);
  // Expect UpdateErrorStatus NOT to be called on the diagnostic service for stop errors
  EXPECT_CALL(*mock_diagnostic_service, UpdateErrorStatus(::testing::_)).Times(0);

  OtamMain otam_main(mock_services_vector);
  otam_main.Initialize();
  otam_main.Start();
  otam_main.Stop();
}

int main(int argc, char **argv) {
  // The testing::InitGoogleTest and testing::InitGoogleMock functions
  // initialize the GTest and GMock frameworks.
  ::testing::InitGoogleMock(&argc, argv); // Initializes GMock (which also initializes GTest)
  // Or: ::testing::InitGoogleTest(&argc, argv); if only using GTest

  // Runs all registered tests.
  return RUN_ALL_TESTS();
}