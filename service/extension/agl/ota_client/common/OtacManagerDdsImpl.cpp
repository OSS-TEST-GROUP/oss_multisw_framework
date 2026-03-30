#include "OtacManagerDdsImpl.hpp"

#include <thread>

using namespace eprosima::fastdds::dds;

namespace ota_client::agl
{

    OtacManagerDdsImpl::OtacManagerDdsImpl(DomainParticipant *participant, Publisher *publisher, Subscriber *subscriber, TypeSupport type)
        : participant_(participant), publisher_(publisher), subscriber_(subscriber), type_(type) {}

    bool OtacManagerDdsImpl::Create(const std::string &topic, bool create_writer, bool create_reader, bool transient_local)
    {

        type_.register_type(participant_);

        if (!topic_) {
            if (auto *td = participant_->lookup_topicdescription(topic)) {
                if (auto *t = dynamic_cast<Topic *>(td)) {
                    topic_ = t;
                } else {
                    std::cerr << "Existing topicdesc is not a Topic\n";
                    return false;
                }
            } else {
                topic_ = participant_->create_topic(topic, type_.get_type_name(), TOPIC_QOS_DEFAULT);
                if (!topic_)
                    return false;
            }
        }

        // topic_ = participant_->create_topic(
        //     topic,
        //     type_.get_type_name(),
        //     TOPIC_QOS_DEFAULT);

        // if (topic_ == nullptr)
        //     return false;

        if (create_writer)
        {
            DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
            if (transient_local) {
                writer_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            }
            data_writer_ = publisher_->create_datawriter(topic_, writer_qos, nullptr);
            if (data_writer_ == nullptr)
                return false;
        }
        if (create_reader)
        {
            DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
            if (transient_local) {
                reader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            }
            listener_ = std::make_unique<ReaderListener>(*this);
            data_reader_ = subscriber_->create_datareader(topic_, reader_qos, listener_.get());
            if (!data_reader_)
                return false;
        }

        return true;
    }

    void OtacManagerDdsImpl::SetQos(uint32_t qos)
    {
    }

    bool OtacManagerDdsImpl::Write(std::shared_ptr<void> message)
    {
        if (!data_writer_) {
            std::cerr << "[OtacManagerDdsImpl] write() no writer\n";
            return false;
        }

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

    void OtacManagerDdsImpl::Close() {
        if (data_writer_) {
            PublicationMatchedStatus ps{};
            for (int i = 0; i < 50; ++i) {
                if (data_writer_->get_publication_matched_status(ps) == RETCODE_OK && ps.current_count == 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        if (data_reader_) {
            SubscriptionMatchedStatus ss{};
            for (int i = 0; i < 50; ++i) {
                if (data_reader_->get_subscription_matched_status(ss) == RETCODE_OK && ss.current_count == 0)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }

    // 삭제 순서: Writer/Reader -> Topic -> (Publisher/Subscriber는 소유자 아님) -> Listener
        if (publisher_ && data_writer_) {
            publisher_->delete_datawriter(data_writer_);
            data_writer_ = nullptr;
        }
        if (subscriber_ && data_reader_) {
            subscriber_->delete_datareader(data_reader_);
            data_reader_ = nullptr;
        }
        if (participant_ && topic_) {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }

        listener_.reset();

    }

    OtacManagerDdsImpl::~OtacManagerDdsImpl() {
        Close();
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