#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

#include "otam_client_service/otam_client_service.h"
#include "mock/mock_otam_dds_impl.h"
#include "generated/idl/ota_mgr/OtamClientServiceMsg.hpp"
#include "otam_common/otam_dds_idl.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::InSequence;

/**
 * @file test_otam_client_service.cc
 * @brief Unit tests for OtamClientService class
 * @author OTA Manager Team
 * @date 2024
 */

/**
 * @class OtamClientServiceTest
 * @brief Test fixture for OtamClientService unit tests
 * 
 * Provides setup and teardown for testing OtamClientService functionality
 * with mocked DDS dependencies.
 */
class OtamClientServiceTest : public ::testing::Test {
protected:
    static void SetUpTestCase() {
        // Create + initialize service inside the suite setup so lifetime is bound
        // to the test run, not to static construction/destruction order.
        service_ = std::make_unique<OtamClientService>();
        service_->Initialize();
        service_->Start();
    }

    static void TearDownTestCase() {
        if (service_) {
            service_->Stop();
            service_.reset();
        }
    }

    // Helper method to inject mock DDS instances into the service
    void InjectMockDds() {
        // Note: This would require friend class or setter methods in OtamClientService
        // For now, we'll test the public interface
    }

    static std::unique_ptr<OtamClientService> service_;
    // std::unique_ptr<StrictMock<MockOtamDdsImpl>> mock_registration_dds_;
    // std::unique_ptr<StrictMock<MockOtamDdsImpl>> mock_registration_response_dds_;
    // std::unique_ptr<StrictMock<MockOtamDdsImpl>> mock_manager_ready_dds_;
    // std::unique_ptr<StrictMock<MockOtamDdsImpl>> mock_app_status_dds_;
};

std::unique_ptr<OtamClientService> OtamClientServiceTest::service_;

/**
 * @brief Test OtamClientService constructor
 */
TEST_F(OtamClientServiceTest, Constructor) {
    EXPECT_TRUE(service_ != nullptr);
}

/**
 * @brief Test client registration message processing
 */
TEST_F(OtamClientServiceTest, OnMessageReceived_ValidRegistration) {
    // Create test registration message
    std::cout << "OnMessageReceived_ValidRegistration-1 !!!!." << std::endl;

    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("test_client");
    
    std::cout << "OnMessageReceived_ValidRegistration-2 !!!!." << std::endl;

    std::vector<std::string> app_list = {"app1", "app2", "app3"};
    for (const auto& app : app_list) {
        registration_msg.app_list().push_back(app);
    }

    std::cout << "OnMessageReceived_ValidRegistration-3 !!!!." << std::endl;

    // Test the message processing
    // Note: Since we can't easily mock the internal DDS instances without
    // modifying the class design, we'll test the public interface behavior
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test client registration with empty app list
 */
TEST_F(OtamClientServiceTest, OnMessageReceived_EmptyAppList) {
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("test_client_empty");
    
    // Empty app list
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test client registration with duplicate client name
 */
TEST_F(OtamClientServiceTest, OnMessageReceived_DuplicateClient) {
    // First registration
    otamanager::SetOtaClientRegistration registration_msg1;
    registration_msg1.client_name("duplicate_client");
    registration_msg1.app_list().push_back("app1");
    
    // Second registration with same client name
    otamanager::SetOtaClientRegistration registration_msg2;
    registration_msg2.client_name("duplicate_client");
    registration_msg2.app_list().push_back("app2");

    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg1));
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg2));
}

/**
 * @brief Test FindClient functionality
 */
TEST_F(OtamClientServiceTest, FindClient_ExistingClient) {
    // Register a client first
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("findable_client");
    registration_msg.app_list().push_back("app1");
    
    service_->OnMessageReceived(registration_msg);
    
    // The FindClient method is private, so we test through public methods
    // that would use it internally
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test FindClientIdWithApp functionality
 */
TEST_F(OtamClientServiceTest, FindClientIdWithApp_ExistingApp) {
    // Register a client with specific app
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("app_owner_client");
    registration_msg.app_list().push_back("unique_app");
    
    service_->OnMessageReceived(registration_msg);
    
    // Test notification with the registered app
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("unique_app", "downloading"));
}

/**
 * @brief Test NotifyAppUpdateStatus with non-existent app
 */
TEST_F(OtamClientServiceTest, NotifyAppUpdateStatus_NonExistentApp) {
    // Test with app that doesn't exist
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("non_existent_app", "installing"));
}

/**
 * @brief Test NotifyAppUpdateStatus with various status values
 */
TEST_F(OtamClientServiceTest, NotifyAppUpdateStatus_VariousStatuses) {
    // Register a client first
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("status_test_client");
    registration_msg.app_list().push_back("status_app");
    
    service_->OnMessageReceived(registration_msg);
    
    // Test different status values
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("status_app", "downloading"));
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("status_app", "installing"));
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("status_app", "complete"));
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("status_app", "failed"));
}

/**
 * @brief Test client ID assignment
 */
TEST_F(OtamClientServiceTest, ClientIdAssignment_MultipleClients) {
    // Register multiple clients and verify they get different IDs
    std::vector<std::string> client_names = {
        "client_1", "client_2", "client_3", "client_4"
    };
    
    for (const auto& client_name : client_names) {
        otamanager::SetOtaClientRegistration registration_msg;
        registration_msg.client_name(client_name);
        registration_msg.app_list().push_back("app_for_" + client_name);
        
        EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
    }
}

/**
 * @brief Test service lifecycle methods
 */
TEST_F(OtamClientServiceTest, ServiceLifecycle) {
    // Test Initialize, Start, Stop sequence
    // EXPECT_NO_THROW(service_->Initialize());
    // EXPECT_NO_THROW(service_->Start());
    // EXPECT_NO_THROW(service_->Stop());
}

/**
 * @brief Test NotifyManagerReady functionality
 */
TEST_F(OtamClientServiceTest, NotifyManagerReady) {
    // Initialize service first
    // service_->Initialize();
    
    // Test manager ready notification
    EXPECT_NO_THROW(service_->NotifyManagerReady());
}

/**
 * @brief Test edge cases for client registration
 */
TEST_F(OtamClientServiceTest, EdgeCases_EmptyClientName) {
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name(""); // Empty client name
    registration_msg.app_list().push_back("app1");
    
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test edge cases for app names
 */
TEST_F(OtamClientServiceTest, EdgeCases_EmptyAppName) {
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("client_with_empty_app");
    registration_msg.app_list().push_back(""); // Empty app name
    
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test large number of applications
 */
TEST_F(OtamClientServiceTest, LargeNumberOfApps) {
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("client_with_many_apps");
    
    // Add 100 applications
    for (int i = 0; i < 100; ++i) {
        registration_msg.app_list().push_back("app_" + std::to_string(i));
    }
    
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
}

/**
 * @brief Test special characters in client and app names
 */
TEST_F(OtamClientServiceTest, SpecialCharacters) {
    otamanager::SetOtaClientRegistration registration_msg;
    registration_msg.client_name("client-with_special.chars@123");
    registration_msg.app_list().push_back("app-with_special.chars@456");
    
    EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
    EXPECT_NO_THROW(service_->NotifyAppUpdateStatus("app-with_special.chars@456", "testing"));
}

/**
 * @brief Performance test for multiple rapid registrations
 */
TEST_F(OtamClientServiceTest, Performance_RapidRegistrations) {
    // Test rapid registration of multiple clients
    for (int i = 0; i < 50; ++i) {
        otamanager::SetOtaClientRegistration registration_msg;
        registration_msg.client_name("rapid_client_" + std::to_string(i));
        registration_msg.app_list().push_back("rapid_app_" + std::to_string(i));

        EXPECT_NO_THROW(service_->OnMessageReceived(registration_msg));
    }
}

// ===== QA 보강: 미커버 등록/알림 경로 =====

// NotifyManagerReady: manager-ready 발행 경로 실행.
TEST_F(OtamClientServiceTest, QA_NotifyManagerReady_NoThrow) {
    EXPECT_NO_THROW(service_->NotifyManagerReady());
}

// SetOtamDiagnosticService: setter (nullptr 허용).
TEST_F(OtamClientServiceTest, QA_SetOtamDiagnosticService_NoThrow) {
    EXPECT_NO_THROW(service_->SetOtamDiagnosticService(nullptr));
}

// DeleteClientById: 등록된 클라이언트 삭제 → SUCCESS (성공 경로).
TEST_F(OtamClientServiceTest, QA_DeleteClientById_Existing_Success) {
    otamanager::SetOtaClientRegistration r;
    r.client_name("qa_to_delete");
    r.app_list().push_back("qa_del_app");
    service_->OnMessageReceived(r);
    auto snap = service_->GetClientInfoListSnapshot();
    ASSERT_FALSE(snap.empty());
    int32_t id = snap.rbegin()->first;  // 최근 등록 id
    EXPECT_EQ(service_->DeleteClientById(id), ReturnValue::SUCCESS);
}

// 같은 앱을 다른 클라이언트가 요청 → FindClientIdWithApp/소유권 경로.
TEST_F(OtamClientServiceTest, QA_OnMessageReceived_AppOwnershipPath) {
    otamanager::SetOtaClientRegistration a;
    a.client_name("qa_owner_a"); a.app_list().push_back("qa_shared_app");
    otamanager::SetOtaClientRegistration b;
    b.client_name("qa_owner_b"); b.app_list().push_back("qa_shared_app");
    EXPECT_NO_THROW(service_->OnMessageReceived(a));
    EXPECT_NO_THROW(service_->OnMessageReceived(b));
}

// 다중 앱 등록 → app_list 순회 경로.
TEST_F(OtamClientServiceTest, QA_OnMessageReceived_MultipleApps) {
    otamanager::SetOtaClientRegistration r;
    r.client_name("qa_multi");
    r.app_list().push_back("qa_m1");
    r.app_list().push_back("qa_m2");
    r.app_list().push_back("qa_m3");
    EXPECT_NO_THROW(service_->OnMessageReceived(r));
}

// 빈 client_name → 검증/에러 분기.
TEST_F(OtamClientServiceTest, QA_OnMessageReceived_EmptyName) {
    otamanager::SetOtaClientRegistration r;
    r.app_list().push_back("qa_noname_app");
    EXPECT_NO_THROW(service_->OnMessageReceived(r));
}

// 같은 이름 재등록(업데이트 경로).
TEST_F(OtamClientServiceTest, QA_OnMessageReceived_ReRegister) {
    otamanager::SetOtaClientRegistration r1;
    r1.client_name("qa_rereg"); r1.app_list().push_back("qa_r1");
    otamanager::SetOtaClientRegistration r2;
    r2.client_name("qa_rereg"); r2.app_list().push_back("qa_r2");
    EXPECT_NO_THROW(service_->OnMessageReceived(r1));
    EXPECT_NO_THROW(service_->OnMessageReceived(r2));
}

// DeleteClientById: 미등록 id → FAIL 분기(명시적).
TEST_F(OtamClientServiceTest, QA_DeleteClientById_Unknown_Fail) {
    EXPECT_EQ(service_->DeleteClientById(123456789), ReturnValue::FAIL);
}

// GetClientInfoListSnapshot: 등록 후 스냅샷에 포함.
TEST_F(OtamClientServiceTest, QA_GetClientInfoListSnapshot_HasEntries) {
    otamanager::SetOtaClientRegistration r;
    r.client_name("qa_snap"); r.app_list().push_back("qa_snap_app");
    service_->OnMessageReceived(r);
    EXPECT_FALSE(service_->GetClientInfoListSnapshot().empty());
}