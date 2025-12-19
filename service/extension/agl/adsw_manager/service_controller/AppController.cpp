#include "AppController.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>   // fork, execvp
#include <sys/wait.h> // waitpid
#include <cstdio>     // popen, fgets, pclose
#include <algorithm>

using ForkFn = pid_t(*)();
using ExecvpFn = int(*)(const char*, char* const*);
using WaitpidFn = pid_t(*)(pid_t, int*, int);
using ExitFn = void(*)(int);

ForkFn g_fork = ::fork;
ExecvpFn g_execvp = ::execvp;
WaitpidFn g_waitpid = ::waitpid;
ExitFn g_exit = ::_exit;

pid_t Fork() { return g_fork(); }
int Execvp(const char* f, char* const* a) { return g_execvp(f, a); }
pid_t Waitpid(pid_t p, int* s, int o) { return g_waitpid(p, s, o); }
void ExitNow(int c) { g_exit(c); }

#ifdef UNIT_TEST
namespace adsw_manager::agl::testhook {
  void SetFork(ForkFn f){ g_fork=f; }
  void SetExecvp(ExecvpFn f){ g_execvp=f; }
  void SetWaitpid(WaitpidFn f){ g_waitpid=f; }
  void SetExit(ExitFn f){ g_exit=f; }
  void ResetAll(){ g_fork=::fork; g_execvp=::execvp; g_waitpid=::waitpid; g_exit=::_exit; }
}
#endif

namespace adsw_manager::agl
{

    bool AppController::Initialize()
    {

        std::cout << "I am AppController!!" << std::endl;

        return true;
    }

    bool AppController::LaunchApp(std::string app_name) {

        int st = RunDocker({"container", "inspect", app_name});
        bool exists = (st >= 0 && WIFEXITED(st) && WEXITSTATUS(st) == 0);

        if (exists)
        {
            int rc = RunDocker({"start", app_name});
            return (rc >= 0 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0);
        }
        
        int rc = RunDocker({
            "run", "-d",
            "--name", app_name,
            "--net=host",
            "--device", "/dev/dri:/dev/dri",
            "-v", "/run/user/1001:/run/user/1001",
            "-v", "$(pwd)/config.json:/etc/adsw/config.json:ro",
            "-e", "ADSW_CONFIG=/etc/adsw/config.json",
            "--privileged",
            "registry.obigo.com/oss/oss-hmi-agl-aarch64:latest" 
        });

        return (rc >= 0 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0);

        // std::string cmd = "/usr/bin/flutter-auto -b /usr/share/flutter/" + app_name + "/3.24.2/release --xdg-shell-app-id " + app_name;
        // int result = std::system(cmd.c_str());

        // if(result == -1) {
        //     std::cout << "Command execution faile. system() returned -1." << std::endl;
        //     return false;
        // }

        // int exit_code = WEXITSTATUS(result);
        // if(exit_code == 0) {
        //     std::cout << app_name << " launched successfully." << std::endl;
        //     return true;
        // } else {
        //     std::cout << "Command execution failed (exit code: " << exit_code << ")" << std::endl;
        //     return false;
        // }
    }

    bool AppController::TerminateApp(std::string app_name)
    {
        int rc = RunDocker({"stop", app_name});
        return (rc >= 0 && WIFEXITED(rc) && WEXITSTATUS(rc) == 0);

        // std::string cmd = "pkill flutter-auto";
        // int result = std::system(cmd.c_str());

        // if(result == -1) {
        //     std::cout << "Command execution failed. system() returned -1." << std::endl;
        //     return false;
        // }

        // int exit_code = WEXITSTATUS(result);
        // if(exit_code == 0) {
        //     std::cout << app_name << " terminated successfully." << std::endl;
        //     return true;
        // } else {
        //     std::cout << "Command execution failed (exit code: " << exit_code << ")" << std::endl;
        //     return false;
        // }
    }

    bool AppController::IsAppRunning(std::string app_name)
    {

        std::string out;
        if (!ReadDockerOutput(
                "docker inspect -f '{{.State.Running}}' " + app_name + " 2>/dev/null",
                out))
        {
            return false;
        }

        return (out.find("true") != std::string::npos);

        // const std::string unit = ToServiceUnit(app_name);

        // std::string state;
        // bool exists = false;
        // if (!GetUnitActiveStateSystem(unit, state, exists))
        // {
        //     std::cerr << "[systemctl] failed to query ActiveState for " << unit << "\n";
        //     return false;
        // }
        // if (!exists)
        // {
        //     std::cout << unit << " is not loaded (system unit not found).\n";
        //     return false;
        // }
        
        // if (state == "active" || state == "activating")
        // {
        //     std::cout << "AppController::IsAppRunning -> " << unit
        //               << " state=" << state << " (system)\n";
        //     return true;
        // }
        // std::cout << unit << " state=" << state << " (not running, system)\n";
        // return false;

        // // std::string cmd = "ps -a | grep -q " + app_name;
        // // int result = std::system(cmd.c_str());

        // // if(result == -1) {
        // //     std::cout << "Command execution failed: system() returned -1." << std::endl;
        // //     return false;
        // // }

        // // int exit_code = WEXITSTATUS(result);
        // // if(exit_code == 0) {
        // //     std::cout << "AppController::IsAppRunning" << std::endl;
        // //     return true;
        // // } else if (exit_code == 1) {
        // //     std::cout << app_name << " is not running." << std::endl;
        // //     return false;
        // // }
        // // std::cout << "Subcommand failed  (exit code: " << exit_code << ")" << std::endl;
        // // return false;
    }

    int AppController::RunDocker(const std::vector<std::string> &args)
    {

        std::vector<char *> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char *>("docker"));
        for (const auto &a : args)
            argv.push_back(const_cast<char *>(a.c_str()));
        argv.push_back(nullptr);

        pid_t pid = Fork();
        //pid_t pid = fork();
        if (pid < 0)
        {
            std::cerr << "[AppController] fork failed: " << strerror(errno) << "\n";
            return -1;
        }

        if (pid == 0)
        {
            // execvp("docker", argv.data());
            // perror("[AppController] execvp failed");
            // _exit(127);
            Execvp("docker", argv.data());
            perror("[AppController] execvp failed");
            ExitNow(127);
        }

        int status = 0;
        //if (waitpid(pid, &status, 0) < 0)
        if (Waitpid(pid, &status, 0) < 0)
        {
            std::cerr << "[AppController] waitpid failed: " << strerror(errno) << "\n";
            return -1;
        }

        return status;
    }

    bool AppController::ReadDockerOutput(const std::string &cmd, std::string &out)
    {

        out.clear();

        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "[docker] popen failed: " << strerror(errno) << "\n";
            return false;
        }

        char buf[256];
        while (fgets(buf, sizeof(buf), pipe))
        {
            out.append(buf);
        }

        pclose(pipe);
        return true;
    }

}