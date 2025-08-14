#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include <string>

namespace adsw_manager::agl {

    class AppController {

        public:
            virtual bool Initialize();
            virtual bool LaunchApp(std::string appId, std::string contName);
            virtual bool TerminateApp(std::string appId, std::string contName);
            virtual bool IsAppRunning(std::string appId, std::string contName);

    };

} // namespace adsw_manager::agl

#endif