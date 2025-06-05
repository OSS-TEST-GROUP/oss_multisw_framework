#include "Service.hpp"

namespace lifecycle_manager {

Service::Service() {}
Service::~Service() {}


std::string Service::GetName(){


}

ServiceType Service::GetServiceType(){


}
RecoveryPolicy Service::GetRecoveryPolicy(){


}
bool Service::isEnabled(){


}

ServiceMode Service::GetServiceMode(){


}
void Service::SetServiceMode(ServiceMode mode){


}

ServiceState Service::GetServiceState(){


}
void Service::SetServiceState(ServiceState state){


}
ServiceState Service::GetLastServiceState(){


}


int32_t Service::GetRecoverdCount(){


}
void Service::IncreateRecoverdCount(){


}
void Service::ClearRecoverdCount(){


}

} // namespace lifecycle_manager
