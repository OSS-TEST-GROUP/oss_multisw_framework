#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include "service_registry/ServiceRegistry.hpp"

#include <string>
#include <vector>

namespace adsw_manager::agl {

    class AppController {

        public:
            virtual bool Initialize();
            virtual bool LaunchApp(std::string app_name);
            virtual bool TerminateApp(std::string app_name);
            virtual bool IsAppRunning(std::string app_name);

            void SetServiceRegistry(ServiceRegistry* registry);

        private:
            ServiceRegistry* service_registry_ {nullptr};

            int RunDocker(const std::vector<std::string>& args);
            bool ReadDockerOutput(const std::string& cmd, std::string& out);

    };

} // namespace adsw_manager::agl

#ifdef UNIT_TEST
namespace adsw_manager::agl::testhook {
  using ForkFn = pid_t(*)();
  using ExecvpFn = int(*)(const char*, char* const*);
  using WaitpidFn = pid_t(*)(pid_t, int*, int);
  using ExitFn = void(*)(int);

  void SetFork(ForkFn);
  void SetExecvp(ExecvpFn);
  void SetWaitpid(WaitpidFn);
  void SetExit(ExitFn);
  void ResetAll();
}
#endif

#endif