#include "DDSTransmitter.hpp"

namespace lifecycle_manager {

DDSTransmitter::DDSTransmitter() {}
DDSTransmitter::~DDSTransmitter() {}


bool DDSTransmitter::Initialize(){

}


void DDSTransmitter::Finalize(){

}


bool DDSTransmitter::AddMessageEndpoint(std::shared_ptr<DDSMessageEndPoint>){

}

std::shared_ptr<DDSMessageEndPoint> DDSTransmitter::GetMessageEndPoint(std::string topic){

    
}



} // namespace lifecycle_manager
