#include "AppController.hpp"
#include <iostream>

namespace adsw_manager::agl {

    bool AppController::Initialize() {

        std::cout << "I am AppController!!" << std::endl;

        return true;

    }

    bool AppController::LaunchApp(std::string appId, std::string contName) {

        std::string cmd = "docker exec -d " + contName + " /usr/bin/flutter-auto -b /usr/share/flutter/" + appId + "/3.24.2/release --xdg-shell-app-id " + appId + "";
        int result = std::system(cmd.c_str());

        if(result == -1) {
            std::cout << "Command execution faile. system() returned -1." << std::endl;
            return false;
        }

        int exit_code = WEXITSTATUS(result);
        if(exit_code == 0) {
            std::cout << appId << " launched successfully." << std::endl;
            return true;
        } else {
            std::cout << "Command execution failed (exit code: " << exit_code << ")" << std::endl;
            return false;
        }

    }

    bool AppController::TerminateApp(std::string appId, std::string contName) {

        std::string cmd = "docker exec " + contName + " pkill flutter-auto";
        int result = std::system(cmd.c_str());

        if(result == -1) {
            std::cout << "Command execution failed. system() returned -1." << std::endl;
            return false;
        }

        int exit_code = WEXITSTATUS(result);
        if(exit_code == 0) {
            std::cout << appId << " terminated successfully." << std::endl;
            return true;
        } else {
            std::cout << "Command execution failed (exit code: " << exit_code << ")" << std::endl;
            return false;
        }

    }

    bool AppController::IsAppRunning(std::string appId, std::string contName) {

        //std::string cmd = "docker exec " + contName + " bash -c \'xeyes\'";

        std::string cmd = "docker exec " + contName + " bash -c \'ps -a | grep -q " + appId + "\'";
        int result = std::system(cmd.c_str());

        if(result == -1) {
            std::cout << "Command execution failed: system() returned -1." << std::endl;
            return false;
        }

        int exit_code = WEXITSTATUS(result);
        if(exit_code == 0) {
            std::cout << "AppController::IsAppRunning" << std::endl;
            return true;
        } else if (exit_code == 1) {
            std::cout << appId << " is not running." << std::endl;
            return false;
        }
        std::cout << "Subcommand failed  (exit code: " << exit_code << ")" << std::endl;
        return false;

    }
    
}