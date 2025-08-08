#ifndef CONTAINER_CONTROLLER_HPP_
#define CONTAINER_CONTROLLER_HPP_

#include <string>

namespace adsw_manager::agl {

    class ContainerController {

        public:
            virtual bool Initialize();
            virtual bool StartContainer(std::string name);
            virtual bool StopContainer(std::string name);
            virtual bool IsContainerRunning(std::string name);

    };

} // namespace adsw_manager::agl

#endif