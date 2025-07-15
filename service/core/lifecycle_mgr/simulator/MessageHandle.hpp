#ifndef FASTDDS_EXAMPLES_CPP_HELLO_WORLD__MESSAGEHANDLE_HPP
#define FASTDDS_EXAMPLES_CPP_HELLO_WORLD__MESSAGEHANDLE_HPP


#include "lifecycle/LifecycleDiagReport.hpp"
#include "lifecycle/SetServiceModeRequest.hpp"
#include "lifecycle/UpdateServiceListRequest.hpp"

using namespace eprosima::fastdds::dds;

namespace eprosima {
namespace fastdds {
namespace examples {
namespace hello_world {

class MessageHandle
{
public:

    MessageHandle() = delete;

    static void PrintMessage(LifecycleDiagReport& message){
        std::vector<::ServiceStatus> *slist = &message.service_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            std::cout << "service " << svc->name() 
                        << ", " << static_cast<int32_t>(svc->service_type()) 
                        << ", " << static_cast<int32_t>(svc->service_state()) 
                        << ", " << static_cast<bool>(svc->is_enabled()) << std::endl;
        }
    }
    static void PrintMessage(GetServiceStateRequest&  message){
        
    }
    static void PrintMessage(GetServiceStateResponse& message){
        
    }
    static void PrintMessage(NotifyServiceState& message){
        
    }
    static void PrintMessage(SetServiceModeRequest& message){
        
    }
    static void PrintMessage(SetServiceModeResponse& message){
        
    }
    static void PrintMessage(StartServiceRequest& message){
        
    }
    static void PrintMessage(StartServiceResponse& message){
        
    }
    static void PrintMessage(StopServiceRequest& message){
        
    }
    static void PrintMessage(StopServiceResponse& message){
        
    }    
    static void PrintMessage(UpdateServiceListRequest& message){

        std::vector<::Service> *slist = &message.service_list();

        for( auto svc = slist->begin(); svc != slist->end(); ++svc ){
            std::cout << "service " << svc->name() 
                        << ", " << static_cast<int32_t>(svc->service_type()) 
                        << ", " << static_cast<int32_t>(svc->recovery_policy()) 
                        << ", " << static_cast<bool>(svc->is_enabled()) << std::endl;

            //GetServieManager()->UpdateService();
        }
    }
    static void PrintMessage(UpdateServiceListResponse& message){
        
    }

    static void SetMessage(LifecycleDiagReport& message){

        ::ServiceStatus value1, value2, value3;

        value1.name() = "service1";
        value1.service_type() = ServiceType::NORMAL;
        value1.service_state() = ServiceState::RUNNING;
        value1.is_enabled() = true;
        value1.recovered_count() = 1;        

        value2.name() = "service2";
        value2.service_type() = ServiceType::NORMAL;
        value2.service_state() = ServiceState::RUNNING;
        value2.is_enabled() = true;
        value2.recovered_count() = 0;        

        value3.name() = "service3";
        value3.service_type() = ServiceType::ONESHOT;
        value3.service_state() = ServiceState::NOT_RUNNING;
        value3.is_enabled() = true;
        value3.recovered_count() = 1;        


        message.service_list().push_back(value1);
        message.service_list().push_back(value2);
        message.service_list().push_back(value3);
    }

    static void SetMessage(SetServiceModeRequest& message){

        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");

        message.is_group() = true;
        message.service_mode() = ServiceMode::NORMAL;

    }
    static void SetMessage(SetServiceModeResponse& message){

        message.ret_code() = ::ReturnCode::SUCCESS;
    }

    static void SetMessage(UpdateServiceListRequest& message){

        ::Service value1, value2, value3;
        value1.name() = "service1";
        value1.service_type() = ServiceType::NORMAL;
        value1.recovery_policy() = RecoveryPolicy::ALWAYS;
        value1.is_enabled() = true;

        value2.name() = "service2";
        value2.service_type() = ServiceType::NORMAL;
        value2.recovery_policy() = RecoveryPolicy::ON_SUCCESS;
        value2.is_enabled() = true;

        value3.name() = "service3";
        value3.service_type() = ServiceType::ONESHOT;
        value3.recovery_policy() = RecoveryPolicy::MANUAL;
        value3.is_enabled() = true;

        message.service_list().push_back(value1);
        message.service_list().push_back(value2);
        message.service_list().push_back(value3);
    }
    static void SetMessage(UpdateServiceListResponse& message){ 
        message.ret_code() = ::ReturnCode::SUCCESS;
    }

    static void SetMessage(GetServiceStateRequest& message){

        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");

        message.is_all_service() = true;
    }
    static void SetMessage(GetServiceStateResponse& message){

        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");

        message.service_state_list().push_back(::ServiceState::RUNNING);
        message.service_state_list().push_back(::ServiceState::RUNNING_NO_RESPONSE);
        message.service_state_list().push_back(::ServiceState::RUNNING);
    }
    static void SetMessage(NotifyServiceState& message){

        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");

        message.service_state_list().push_back(::ServiceState::RUNNING);
        message.service_state_list().push_back(::ServiceState::RUNNING_NO_RESPONSE);
        message.service_state_list().push_back(::ServiceState::RUNNING);

    }    
  
    static void SetMessage(StartServiceRequest& message){

        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");

        message.is_group() = true;
    }
    static void SetMessage(StartServiceResponse& message){

        message.ret_code() = ::ReturnCode::SUCCESS;
    }

    static void SetMessage(StopServiceRequest& message){
        message.service_list().push_back("service1");
        message.service_list().push_back("service2");
        message.service_list().push_back("service3");
    }
    static void SetMessage(StopServiceResponse& message){

        message.ret_code() = ::ReturnCode::SUCCESS;

    }    
    
private:

};

} // namespace hello_world
} // namespace examples
} // namespace fastdds
} // namespace eprosima

#endif // FASTDDS_EXAMPLES_CPP_HELLO_WORLD__MESSAGEHANDLE_HPP
