#ifndef OTAC_MANAGER_DDS_IMPL_HPP_
#define OTAC_MANAGER_DDS_IMPL_HPP_

#include <memory>
#include <mutex>
#include "DdsAbstraction.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace ota_client::agl {

    class OtacManagerDdsImpl : public DdsAbstraction {
        public:
            using OnMessageCB = DdsAbstraction::OnMessageCB;

            OtacManagerDdsImpl(eprosima::fastdds::dds::DomainParticipant* participant,
                               eprosima::fastdds::dds::Publisher* publisher,
                               eprosima::fastdds::dds::Subscriber* subscriber,
                               eprosima::fastdds::dds::TypeSupport type);
            bool Create(const std::string& topic, bool create_writer = true, bool create_reader = true) override;
            void SetQos(uint32_t qos) override;
            bool Write(std::shared_ptr<void> message) override;
            // void Read(std::shared_ptr<void> message) override;
            void SetOnMessage(OnMessageCB cb);

        private:
            class ReaderListener : public eprosima::fastdds::dds::DataReaderListener {
                public:
                    explicit ReaderListener(OtacManagerDdsImpl& owner) : owner_(owner) {}
                    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;
                private:
                    OtacManagerDdsImpl& owner_;
            };

            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Publisher* publisher_{nullptr};
            eprosima::fastdds::dds::Subscriber* subscriber_{nullptr};
            eprosima::fastdds::dds::DataWriter* data_writer_{nullptr};
            eprosima::fastdds::dds::DataReader* data_reader_{nullptr};
            eprosima::fastdds::dds::Topic* topic_{nullptr};
            eprosima::fastdds::dds::TypeSupport type_;
            std::unique_ptr<ReaderListener> listener_;

            OnMessageCB on_message_;
            std::mutex mtx_;

    };
};

#endif