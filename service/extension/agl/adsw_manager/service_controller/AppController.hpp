#ifndef APP_CONTROLLER_HPP_
#define APP_CONTROLLER_HPP_

#include <string>

namespace adsw_manager::agl {

    class AppController {

        public:
            virtual bool Initialize();
            virtual bool LaunchApp(std::string app_name);
            virtual bool TerminateApp(std::string app_name);
            virtual bool IsAppRunning(std::string app_name);

    };

} // namespace adsw_manager::agl

#ifdef UNIT_TEST
#include <sys/types.h> // pid_t

namespace adsw_manager::agl::testhook {

// AppController.cpp 내부 훅과 동일한 시그니처로 typedef 제공
using ForkFn    = pid_t(*)();
using ExecvpFn  = int(*)(const char*, char* const*);
using WaitpidFn = pid_t(*)(pid_t, int*, int);
using ExitFn    = void(*)(int);

// 훅 설정/리셋 (AppController.cpp에서 정의됨)
void SetFork(ForkFn f);
void SetExecvp(ExecvpFn f);
void SetWaitpid(WaitpidFn f);
void SetExit(ExitFn f);
void ResetAll();

} // namespace adsw_manager::agl::testhook
#endif // UNIT_TEST

#endif