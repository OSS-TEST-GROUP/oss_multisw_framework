#ifndef MOCK_LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_
#define MOCK_LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_

#include <gmock/gmock.h>
#include "transmitter/DDSMessageEndPoint.hpp"

namespace lifecycle_manager {
class MockDDSMessageEndPoint : public DDSMessageEndPoint {
public:
    MockDDSMessageEndPoint(std::string topic) : DDSMessageEndPoint(topic) {}
    ~MockDDSMessageEndPoint() = default;

    // Only mock virtual methods
    MOCK_METHOD(void, on_publication_matched, (DataWriter* writer, const PublicationMatchedStatus& status), ());
    MOCK_METHOD(void, on_subscription_matched, (DataReader* reader, const SubscriptionMatchedStatus& status), ());
    MOCK_METHOD(void, on_data_available, (DataReader* reader), ());
    MOCK_METHOD(bool, Write, (const void* const data), ());
    MOCK_METHOD(bool, Initialize, (DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber), ());
    MOCK_METHOD(void, Finalize, (), ());
    MOCK_METHOD(std::string, GetTopic, (), ());
    MOCK_METHOD(std::string, GetTypeName, (), ());
    MOCK_METHOD(std::shared_ptr<TypeSupport>, GetTypeSupport, (), ());
    MOCK_METHOD(void, SetTypeSupport, (std::shared_ptr<TypeSupport> typesupport), ());
    MOCK_METHOD(void, OnDataPublished, (bool ret), ());
    MOCK_METHOD(void, OnDataAvailable, (), ());
};

inline MockDDSMessageEndPoint* Mock_DDSMessageEndPoint = nullptr;



DDSMessageEndPoint::DDSMessageEndPoint(std::string topic) : MessageEndPoint(topic) {
}
DDSMessageEndPoint::~DDSMessageEndPoint() {
    // Don't set to nullptr here as multiple endpoints may exist
}
bool DDSMessageEndPoint::Initialize(DomainParticipant* participant, Publisher* publisher, Subscriber* subscriber) {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        return Mock_DDSMessageEndPoint->Initialize(participant, publisher, subscriber);
    }
    // Real implementation - return true for tests
    return true;
}
void DDSMessageEndPoint::Finalize() {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        Mock_DDSMessageEndPoint->Finalize();
    }
}
#if 0
std::string DDSMessageEndPoint::GetTopic() {
    return Mock_DDSMessageEndPoint->GetTopic(); 
}

std::string DDSMessageEndPoint::GetTypeName() {
    return Mock_DDSMessageEndPoint->GetTypeName();      
}
std::shared_ptr<TypeSupport> DDSMessageEndPoint::GetTypeSupport() {
    return Mock_DDSMessageEndPoint->GetTypeSupport();
}
void DDSMessageEndPoint::SetTypeSupport(std::shared_ptr<TypeSupport> typesupport) {
    Mock_DDSMessageEndPoint->SetTypeSupport(typesupport);   
}


void DDSMessageEndPoint::OnDataPublished(bool ret) {
    Mock_DDSMessageEndPoint->OnDataPublished(ret);      
}
void DDSMessageEndPoint::OnDataAvailable() {
    Mock_DDSMessageEndPoint->OnDataAvailable();   
}
#endif

void DDSMessageEndPoint::on_publication_matched(DataWriter* writer, const PublicationMatchedStatus& status) {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        Mock_DDSMessageEndPoint->on_publication_matched(writer, status);
    }
}
void DDSMessageEndPoint::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& status) {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        Mock_DDSMessageEndPoint->on_subscription_matched(reader, status);
    }
}
void DDSMessageEndPoint::on_data_available(DataReader* reader) {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        Mock_DDSMessageEndPoint->on_data_available(reader);
    }
}
bool DDSMessageEndPoint::Write(const void* const data) {
    if (Mock_DDSMessageEndPoint && Mock_DDSMessageEndPoint != this) {
        return Mock_DDSMessageEndPoint->Write(data);
    }
    return true;
}

} // namespace lifecycle_manager

#endif // MOCK_LIFECYCLE_DDS_MESSAGE_ENDPOINT_HPP_