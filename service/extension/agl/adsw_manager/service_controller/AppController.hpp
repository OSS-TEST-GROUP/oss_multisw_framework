#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include <string>
#include <vector>

namespace adsw_manager::agl {

    class AppController {

        public:
            virtual bool Initialize();
            virtual bool LaunchApp(std::string app_name);
            virtual bool TerminateApp(std::string app_name);
            virtual bool IsAppRunning(std::string app_name);

        private:
            int RunDocker(const std::vector<std::string>& args);
            bool ReadDockerOutput(const std::string& cmd, std::string& out);

    };

} // namespace adsw_manager::agl

#endif