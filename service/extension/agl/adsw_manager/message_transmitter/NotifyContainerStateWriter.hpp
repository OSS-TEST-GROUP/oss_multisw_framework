#ifndef NOTIFY_CONTAINER_STATE_WRITER_HPP_
#define NOTIFY_CONTAINER_STATE_WRITER_HPP_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "lifecycle/NotifyContainerStatePubSubTypes.hpp"

namespace adsw_manager::agl {

    // F009 (C-1): publishes lifecyclemgr::NotifyContainerState for the docker
    // app-containers agl_adsw_manager manages, so lfc_monitor renders AGL apps
    // as container cards (lifecycle_mgr forwards it; dds_bridge bridges it to
    // rosbridge). Mirrors NotifyAppStateWriter's lifecycle.
    //
    // NOTE: on a domain shared with the ros2 adsw_manager (which also publishes
    // NotifyContainerState) there are two publishers on this topic — the
    // dds_bridge trust-on-first-use pins one. Clean cross-manager aggregation is
    // tracked under F008. On a real AGL target (Orin), agl_adsw_manager is the
    // sole container manager so there is no contention.
    class NotifyContainerStateWriter {
        public:
            NotifyContainerStateWriter() = default;
            virtual ~NotifyContainerStateWriter();

            virtual bool Initialize(eprosima::fastdds::dds::Publisher* publisher,
                            eprosima::fastdds::dds::DomainParticipant* participant);
            virtual bool Publish(const lifecyclemgr::NotifyContainerState& msg);

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Publisher* publisher_{nullptr};
            eprosima::fastdds::dds::Topic* topic_{nullptr};
            eprosima::fastdds::dds::DataWriter* writer_{nullptr};
            eprosima::fastdds::dds::TypeSupport type_{nullptr};
    };
};

#endif
