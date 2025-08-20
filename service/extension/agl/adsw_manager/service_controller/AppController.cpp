#include "AppController.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>     // fork, execvp
#include <sys/wait.h>   // waitpid
#include <cstdio>       // popen, fgets, pclose
#include <algorithm>

namespace {

    inline std::string ToServiceUnit(const std::string& app_name) {
        if (app_name.size() >= 8 && app_name.rfind(".service") == app_name.size() - 8)
            return app_name;
        return app_name + ".service";
    }

    inline void Trim(std::string& s) {
        auto not_space = [](unsigned char c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    }

    int RunSystemctl(const std::vector<std::string>& args) {
        // argv 준비
        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>("systemctl"));
        for (const auto& a : args) {
            argv.push_back(const_cast<char*>(a.c_str()));
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "[systemctl] fork failed: " << strerror(errno) << std::endl;
            return -1;
        }
        if (pid == 0) {
            // child
            execvp("systemctl", argv.data());
            // exec 실패 시
            std::perror("[systemctl] execvp failed");
            _exit(127);
        }
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            std::cerr << "[systemctl] waitpid failed: " << strerror(errno) << std::endl;
            return -1;
        }
        return status; // WIFEXITED/WEXITSTATUS로 해석
    }

    bool RunAndCapture(const std::string& cmd, std::string& out) {
        out.clear();
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return false;
        char buf[256];
        while (fgets(buf, sizeof(buf), pipe)) {
            out.append(buf);
        }
        int rc = pclose(pipe);
        (void)rc;
        return true;
    }

    bool StartUnitSystem(const std::string& unit) {
        int st = RunSystemctl({"start", unit});
        if (st < 0) return false;
        if (WIFEXITED(st) && WEXITSTATUS(st) == 0) return true;
        std::cerr << "[systemctl] start " << unit
                  << " failed (exit=" << (WIFEXITED(st)? WEXITSTATUS(st) : -1) << ")\n";
        return false;
    }

    bool StopUnitSystem(const std::string& unit) {
        int st = RunSystemctl({"stop", unit});
        if (st < 0) return false;
        if (WIFEXITED(st) && WEXITSTATUS(st) == 0) return true;
        std::cerr << "[systemctl] stop " << unit
                  << " failed (exit=" << (WIFEXITED(st)? WEXITSTATUS(st) : -1) << ")\n";
        return false;
    }


    bool GetUnitActiveStateSystem(const std::string& unit,
                              std::string& out_state,
                              bool& out_exists) {
        out_state.clear();
        out_exists = false;

        // 존재 여부: LoadState
        {
            std::string out;
            // --value를 쓰면 값만 출력됨
            if (!RunAndCapture("systemctl show " + unit + " -p LoadState --value", out)) {
                return false;
            }
            Trim(out);
            if (out != "loaded") {
                // 로드되지 않았으면 존재하지 않는다고 판단
                out_exists = false;
                out_state = "not-found";
                return true;
            }
            out_exists = true;
        }
        // 상태: ActiveState
        {
            std::string out;
            if (!RunAndCapture("systemctl show " + unit + " -p ActiveState --value", out)) {
                return false;
            }
            Trim(out);
            out_state = out;
            return true;
        }
    }


}

namespace adsw_manager::agl {

    bool AppController::Initialize() {

        std::cout << "I am AppController!!" << std::endl;

        return true;

    }

    bool AppController::LaunchApp(std::string app_name) {
        
        const std::string unit = ToServiceUnit(app_name);
        bool ok = StartUnitSystem(unit);
        if (ok) {
            std::cout << unit << " start requested via systemctl (system unit)." << std::endl;
        }
        return ok;

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

    bool AppController::TerminateApp(std::string app_name) {

        const std::string unit = ToServiceUnit(app_name);
        bool ok = StopUnitSystem(unit);
        if (ok) {
            std::cout << unit << " stop requested via systemctl (system unit)." << std::endl;
        }
        return ok;

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

    bool AppController::IsAppRunning(std::string app_name) {

        const std::string unit = ToServiceUnit(app_name);

        std::string state;
        bool exists = false;
        if (!GetUnitActiveStateSystem(unit, state, exists)) {
            std::cerr << "[systemctl] failed to query ActiveState for " << unit << "\n";
            return false;
        }
        if (!exists) {
            std::cout << unit << " is not loaded (system unit not found).\n";
            return false;
        }
        // 'active'는 실행 중, 'activating'은 기동 중으로 취급(원하면 false로 바꿔도 됨)
        if (state == "active" || state == "activating") {
            std::cout << "AppController::IsAppRunning -> " << unit
                      << " state=" << state << " (system)\n";
            return true;
        }
        std::cout << unit << " state=" << state << " (not running, system)\n";
        return false;

        // std::string cmd = "ps -a | grep -q " + app_name;
        // int result = std::system(cmd.c_str());

        // if(result == -1) {
        //     std::cout << "Command execution failed: system() returned -1." << std::endl;
        //     return false;
        // }

        // int exit_code = WEXITSTATUS(result);
        // if(exit_code == 0) {
        //     std::cout << "AppController::IsAppRunning" << std::endl;
        //     return true;
        // } else if (exit_code == 1) {
        //     std::cout << app_name << " is not running." << std::endl;
        //     return false;
        // }
        // std::cout << "Subcommand failed  (exit code: " << exit_code << ")" << std::endl;
        // return false;
    }
    
}