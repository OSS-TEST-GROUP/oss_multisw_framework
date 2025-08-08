#ifndef NOTIFY_APP_STATE_WRITER_HPP_
#define NOTIFY_APP_STATE_WRITER_HPP_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "agl/NotifyAppStatePubSubTypes.hpp"

namespace adsw_manager::agl {

    class NotifyAppStateWriter {
        public:
            NotifyAppStateWriter() = default;
            ~NotifyAppStateWriter();
            
            bool Initialize(eprosima::fastdds::dds::Publisher* publisher,
                            eprosima::fastdds::dds::DomainParticipant* participant);
            bool Publish(const adsw_manager_agl::NotifyAppState& msg);

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Publisher* publisher_{nullptr};
            eprosima::fastdds::dds::Topic* topic_{nullptr};
            eprosima::fastdds::dds::DataWriter* writer_{nullptr};
            eprosima::fastdds::dds::TypeSupport type_{nullptr};
    };
};

#endif