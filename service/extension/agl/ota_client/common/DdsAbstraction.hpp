#ifndef DDS_ABSTRACTION_HPP_
#define DDS_ABSTRACTION_HPP_

#include <iostream>
#include <memory>
#include <cstdint>
#include <functional>

namespace eprosima { namespace fastdds { namespace dds { struct SampleInfo; }}}

namespace ota_client::agl {

    class DdsAbstraction {
        public:
            using OnMessageCB = 
                std::function<void(std::shared_ptr<void>, const eprosima::fastdds::dds::SampleInfo&)>;
            virtual bool Create(const std::string& topic, bool create_writer = true, bool create_reader = true, bool transient_local = false) = 0;
            virtual void SetQos(uint32_t qos) = 0;
            virtual bool Write(std::shared_ptr<void> message) = 0;
            // virtual void Read(std::shared_ptr<void> message) = 0;
            virtual void SetOnMessage(OnMessageCB cb) = 0;
            virtual ~DdsAbstraction() = default;
    };
};

#endif