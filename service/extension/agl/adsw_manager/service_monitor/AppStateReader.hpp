#ifndef APP_STATE_READER_HPP
#define APP_STATE_READER_HPP

#include <chrono>
#include <string>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

namespace adsw_manager::agl {

    class AppStateReader : public eprosima::fastdds::dds::DataReaderListener {
        public:
            AppStateReader(eprosima::fastdds::dds::DomainParticipant* participant, const std::string& app_name);
            AppStateReader();
            virtual ~AppStateReader();

            virtual std::chrono::steady_clock::time_point GetLastReceivedTime() const;

            void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Subscriber* subscriber_{nullptr};
            eprosima::fastdds::dds::Topic* topic_{nullptr};
            eprosima::fastdds::dds::DataReader* reader_{nullptr};
            eprosima::fastdds::dds::TypeSupport type_{nullptr};

            std::string app_name_;

            std::chrono::steady_clock::time_point last_received_time_;
            mutable std::mutex mutex_;
    };
} // namespace adsw_manager::agl

#endif