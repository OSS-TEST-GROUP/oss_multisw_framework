#ifndef ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_MESSAGE_ENDPOINT_HPP_
#define ROS2_ADSW_MANAGER_UNITTEST_MOCK_MOCK_DDS_MESSAGE_ENDPOINT_HPP_

#include <gmock/gmock.h>

#include "transmitter/DDSMessageEndpoint.hpp"

using namespace eprosima::fastdds::dds;

namespace ros2 {
namespace adsw_manager {


class MockDDSMessageEndpoint : public DDSMessageEndpoint {
  public:
    MockDDSMessageEndpoint() : DDSMessageEndpoint() {}
    ~MockDDSMessageEndpoint() override = default;

    MOCK_METHOD(void, OnDataAvailable, (), (override));
    MOCK_METHOD(bool, Write, (void* value), ());
    MOCK_METHOD(std::string, GetTopic, (), (override));
    MOCK_METHOD(std::string, GetTypeName, (), (override));
    MOCK_METHOD(std::shared_ptr<TypeSupport>, GetTypeSupport, (), (override));
    MOCK_METHOD(bool, Initialize, (DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber), ());
    MOCK_METHOD(void, Finalize, (), ());

    MOCK_METHOD(uint64_t, GetPendingWriteData, (), ());
    MOCK_METHOD(uint64_t, GetPendingReadData, (), ());

    MOCK_METHOD(void, on_publication_matched,
            (DataWriter* writer, const PublicationMatchedStatus& info), (override));

    MOCK_METHOD(void, on_subscription_matched,
            (DataReader* reader, const SubscriptionMatchedStatus& info), (override));

    MOCK_METHOD(void, on_data_available, (DataReader* reader), (override));

    std::shared_ptr<TypeSupport> type_support_;
};

    MockDDSMessageEndpoint* Mock_DDSMessageEndpoint;

    DDSMessageEndpoint::DDSMessageEndpoint(){
    }

    // ~DDSMessageEndpoint() 정의는 제거했다. 프로덕션 헤더가 `= default` 로 바뀌어
    // 아웃오브라인 정의가 위법(explicitly-defaulted 함수의 재정의)이 되고,
    // defaulted 소멸자는 컴파일러가 각 TU 에 만들어주므로 링크 심이 필요 없다.
    bool DDSMessageEndpoint::Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber) {
        return Mock_DDSMessageEndpoint->Initialize(participant, publisher, subscriber);
    }
    void DDSMessageEndpoint::Finalize() {
        Mock_DDSMessageEndpoint->Finalize();
    }


    std::string DDSMessageEndpoint::GetTopic() {
        return Mock_DDSMessageEndpoint->GetTopic();
    }
    std::string DDSMessageEndpoint::GetTypeName() {
        return Mock_DDSMessageEndpoint->GetTypeName();
    }
    
    std::shared_ptr<TypeSupport> DDSMessageEndpoint::GetTypeSupport() {
        return Mock_DDSMessageEndpoint->GetTypeSupport();
    }

    bool DDSMessageEndpoint::Write(void* value) {
        return Mock_DDSMessageEndpoint->Write(value);
    }

    void DDSMessageEndpoint::OnDataAvailable() {
        Mock_DDSMessageEndpoint->OnDataAvailable();
    }

    uint64_t DDSMessageEndpoint::GetPendingWriteData() {
        return Mock_DDSMessageEndpoint->GetPendingWriteData();
    }

    uint64_t DDSMessageEndpoint::GetPendingReadData() {
        return Mock_DDSMessageEndpoint->GetPendingReadData();
    }
    void DDSMessageEndpoint::on_publication_matched(
            DataWriter* writer, const PublicationMatchedStatus& info) {
        Mock_DDSMessageEndpoint->on_publication_matched(writer, info);
    }
    void DDSMessageEndpoint::on_subscription_matched(
            DataReader* reader, const SubscriptionMatchedStatus& info) {
        Mock_DDSMessageEndpoint->on_subscription_matched(reader, info);
    }
    void DDSMessageEndpoint::on_data_available(DataReader* reader) {
        Mock_DDSMessageEndpoint->on_data_available(reader);
    }
}
}

#endif
