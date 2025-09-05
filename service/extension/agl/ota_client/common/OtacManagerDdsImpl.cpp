#include "OtacManagerDdsImpl.hpp"

using namespace eprosima::fastdds::dds;

namespace ota_client::agl
{

    OtacManagerDdsImpl::OtacManagerDdsImpl(DomainParticipant *participant, Publisher *publisher, Subscriber *subscriber, TypeSupport type)
        : participant_(participant), publisher_(publisher), subscriber_(subscriber), type_(type) {}

    bool OtacManagerDdsImpl::Create(const std::string &topic, bool create_writer, bool create_reader)
    {

        type_.register_type(participant_);

        topic_ = participant_->create_topic(
            topic,
            type_.get_type_name(),
            TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
            return false;

        if (create_writer)
        {
            data_writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, nullptr);
            if (data_writer_ == nullptr)
                return false;
        }
        if (create_reader)
        {
            listener_ = std::make_unique<ReaderListener>(*this);
            data_reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, listener_.get());
            if (!data_reader_)
                return false;
        }

        // if (create_reader && data_reader_)
        // {
        //     SubscriptionMatchedStatus s{};
        //     data_reader_->get_subscription_matched_status(s);
        //     std::cout << "[CLIENT] reader matched now=" << s.current_count << "\n";
        // }
        // if (create_writer && data_writer_)
        // {
        //     PublicationMatchedStatus p{};
        //     data_writer_->get_publication_matched_status(p);
        //     std::cout << "[CLIENT] writer matched now=" << p.current_count << "\n";
        // }

        return true;
    }

    void OtacManagerDdsImpl::SetQos(uint32_t qos)
    {
    }

    bool OtacManagerDdsImpl::Write(std::shared_ptr<void> message)
    {

        ReturnCode_t rc = data_writer_->write(message.get());
        if (rc != RETCODE_OK)
        {
            std::cerr << "[OtacManagerDdsImpl] write() failed: " << (int)rc << std::endl;
            return false;
        }
        std::cout << "publish success\n";
        return true;
    }

    // void OtacManagerDdsImpl::Read(std::shared_ptr<void> message) {

    // }

    void OtacManagerDdsImpl::SetOnMessage(OnMessageCB cb)
    {
        on_message_ = std::move(cb);
    }

    void OtacManagerDdsImpl::ReaderListener::on_data_available(DataReader *reader)
    {

        SampleInfo info;
        for (;;)
        {
            TypeSupport ts;
            OnMessageCB cb;
            {
                std::lock_guard<std::mutex> lk(owner_.mtx_);
                if (reader != owner_.data_reader_)
                    return;
                ts = owner_.type_;
                cb = owner_.on_message_;
            }
            if (!ts)
                return;

            void *raw = ts->create_data();
            auto rc = reader->take_next_sample(raw, &info);
            if (rc != RETCODE_OK)
            {
                ts->delete_data(raw);
                break;
            }
            if (!info.valid_data)
            {
                ts->delete_data(raw);
                continue;
            }

            std::shared_ptr<void> sp(raw, [ts](void *p)
                                     { ts->delete_data(p); });
            if (cb)
                cb(std::move(sp), info);
        }
    }

}