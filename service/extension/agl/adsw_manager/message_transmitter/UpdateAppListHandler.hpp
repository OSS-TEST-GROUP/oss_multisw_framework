#ifndef UPDATE_APP_LIST_HANDLER_HPP_
#define UPDATE_APP_LIST_HANDLER_HPP_

#include "lifecycle/UpdateServiceListRequestPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace adsw_manager::agl {
    class UpdateAppListHandler {
        public:
            using RequestCallback = std::function<void(const lifecyclemgr::UpdateServiceListResponse&)>;

            UpdateAppListHandler() = default;
            virtual ~UpdateAppListHandler();

            bool Initialize(eprosima::fastdds::dds::DomainParticipant* participant,
                            eprosima::fastdds::dds::Publisher* publisher,
                            eprosima::fastdds::dds::Subscriber* subscriber,
                            RequestCallback cb);

            virtual bool Publish(const lifecyclemgr::UpdateServiceListRequest& request);

        private:
            class RequestListener : public eprosima::fastdds::dds::DataReaderListener {
                public:
                    explicit RequestListener(RequestCallback cb) : callback_(std::move(cb)) {}

                    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
                        lifecyclemgr::UpdateServiceListResponse msg;
                        eprosima::fastdds::dds::SampleInfo info;
                        if (reader->take_next_sample(&msg, &info) == eprosima::fastdds::dds::RETCODE_OK && info.valid_data) {
                            callback_(msg);
                        }
                    };
                private:
                RequestCallback callback_;
            };

            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Publisher* publisher_{nullptr};
            eprosima::fastdds::dds::Subscriber* subscriber_{nullptr};

            eprosima::fastdds::dds::TypeSupport req_type_;
            eprosima::fastdds::dds::TypeSupport res_type_;

            eprosima::fastdds::dds::Topic* req_topic_{nullptr};
            eprosima::fastdds::dds::Topic* res_topic_{nullptr};

            eprosima::fastdds::dds::DataReader* reader_{nullptr};
            eprosima::fastdds::dds::DataWriter* writer_{nullptr};

            std::unique_ptr<RequestListener> listener_;
    };
} // namespace adsw_manager::agl

#endif // UPDATE_APP_LIST_HANDLER_HPP_