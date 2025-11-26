#ifndef DDS_MANAGER_HPP_
#define DDS_MANAGER_HPP_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include "NotifyAppStateWriter.hpp"
#include "UpdateAppListHandler.hpp"
#include "AppStateHandler.hpp"
#include "StartAppHandler.hpp"
#include "StopAppHandler.hpp"


namespace adsw_manager::agl {

    class DDSManager {

        public:

            DDSManager() = default;
            virtual ~DDSManager();

            virtual bool Initialize(eprosima::fastdds::dds::DomainParticipant* participant);

            virtual void PublishNotifyAppState(const lifecyclemgr::NotifyServiceState& msg);
            virtual void PublishUpdateAppList(const lifecyclemgr::UpdateServiceListRequest& msg);

            virtual void SetOnGetAppStateRequest(std::function<void(const lifecyclemgr::GetServiceStateRequest&)> cb);
            virtual void SetOnStartAppRequest(std::function<void(const lifecyclemgr::StartServiceRequest&)> cb);
            virtual void SetOnStopAppRequest(std::function<void(const lifecyclemgr::StopServiceRequest&)> cb);
            virtual void SetOnUpdateListResponse(std::function<void(const lifecyclemgr::UpdateServiceListResponse&)> cb);

            virtual AppStateHandler* GetAppStateHandler() { return app_state_handler_.get(); }
            virtual StartAppHandler* GetStartAppHandler() { return start_app_handler_.get(); }
            virtual StopAppHandler* GetStopAppHandler() { return stop_app_handler_.get(); }
            virtual UpdateAppListHandler* GetUpdateAppListHandler() { return update_app_list_handler_.get(); }

            #ifdef UNIT_TEST
                void InjectNotifyWriterForTest(std::unique_ptr<NotifyAppStateWriter> w) {
                    notify_app_state_writer_ = std::move(w);
                }
                void InjectUpdateAppListHandlerForTest(std::unique_ptr<UpdateAppListHandler> h) {
                    update_app_list_handler_ = std::move(h);
                }
                void InjectAppStateHandlerForTest(std::unique_ptr<AppStateHandler> a) {
                    app_state_handler_ = std::move(a);
                }
                void InjectStartAppHandlerForTest(std::unique_ptr<StartAppHandler> s) {
                    start_app_handler_ = std::move(s);
                }
                void InjectStopAppHandlerForTest(std::unique_ptr<StopAppHandler> t) {
                    stop_app_handler_ = std::move(t);
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
            std::unique_ptr<UpdateAppListHandler> update_app_list_handler_;
            
            std::function<void(const lifecyclemgr::GetServiceStateRequest&)> on_get_app_state_request_;
            std::function<void(const lifecyclemgr::StartServiceRequest&)> on_start_app_request_;
            std::function<void(const lifecyclemgr::StopServiceRequest&)> on_stop_app_request_;
            std::function<void(const lifecyclemgr::UpdateServiceListResponse&)> on_update_list_response_;

    };

} // namespace adsw_manager::agl

#endif