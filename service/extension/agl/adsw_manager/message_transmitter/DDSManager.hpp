#ifndef DDS_MANAGER_HPP_
#define DDS_MANAGER_HPP_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "NotifyAppStateWriter.hpp"
#include "AppStateHandler.hpp"
#include "StartAppHandler.hpp"
#include "StopAppHandler.hpp"


namespace adsw_manager::agl {

    class DDSManager {

        public:

            DDSManager() = default;
            virtual ~DDSManager();

            virtual bool Initialize(eprosima::fastdds::dds::DomainParticipant* participant);

            virtual void PublishNotifyAppState(const adsw_manager_agl::NotifyAppState& msg);

            virtual void SetOnGetAppStateRequest(std::function<void(const adsw_manager_agl::GetAppStateRequest&)> cb);
            virtual void SetOnStartAppRequest(std::function<void(const adsw_manager_agl::StartAppRequest&)> cb);
            virtual void SetOnStopAppRequest(std::function<void(const adsw_manager_agl::StopAppRequest&)> cb);

            virtual AppStateHandler* GetAppStateHandler() { return app_state_handler_.get(); }
            virtual StartAppHandler* GetStartAppHandler() { return start_app_handler_.get(); }
            virtual StopAppHandler* GetStopAppHandler() { return stop_app_handler_.get(); }

            #ifdef UNIT_TEST
void InjectNotifyWriterForTest(std::unique_ptr<NotifyAppStateWriter> w) {
  notify_app_state_writer_ = std::move(w);
}
#endif

        private:
            eprosima::fastdds::dds::DomainParticipant* participant_{nullptr};
            eprosima::fastdds::dds::Publisher* publisher_{nullptr};
            eprosima::fastdds::dds::Subscriber* subscriber_{nullptr};

            std::unique_ptr<AppStateHandler> app_state_handler_;
            std::unique_ptr<StartAppHandler> start_app_handler_;
            std::unique_ptr<StopAppHandler> stop_app_handler_;

            std::unique_ptr<NotifyAppStateWriter> notify_app_state_writer_;
            
            std::function<void(const adsw_manager_agl::GetAppStateRequest&)> on_get_app_state_request_;
            std::function<void(const adsw_manager_agl::StartAppRequest&)> on_start_app_request_;
            std::function<void(const adsw_manager_agl::StopAppRequest&)> on_stop_app_request_;

    };

} // namespace adsw_manager::agl

#endif