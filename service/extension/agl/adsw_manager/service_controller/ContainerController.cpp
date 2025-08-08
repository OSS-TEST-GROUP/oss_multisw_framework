#include "ContainerController.hpp"
//#include <cstdlib>
#include <iostream>

namespace adsw_manager::agl {

    bool ContainerController::Initialize() {

        std::cout << "I am ContainerController!!" << std::endl;

        return true;

    }

    bool ContainerController::StartContainer(std::string name) {

        std::string cmd = "docker ps -a | grep -q " + name;
        int result = std::system(cmd.c_str());

        if(result == 0) {
            // container is already exist.
            std::string s_cmd = "docker start " + name;
            int s_result = std::system(s_cmd.c_str());

            if(s_result == -1) {
                std::cout << "Command execution failed." << std::endl;
                return false;
            }

            int s_exit_code = WEXITSTATUS(s_result);
            if(s_exit_code == 0) {
                return true;
            } else {
                return false;
            }
        } else if (result == 1) {
            // container is not exist. need to creat docker container.
            std::string c_cmd = "docker run -it --net=host --user $(id -u):$(id -g) \
            -v /run/user/1001:/run/user/1001 --device /dev/dri:/dev/dri \
            -e WAYLAND_DISPLAY=wayland-0 -e XDG_RUNTIME_DIR=/run/user/1001 adsw_monitor_image --name " + name;
            int c_result = std::system(c_cmd.c_str());

            if(c_result == -1) {
                std::cout << "Command execution failed." << std::endl;
                return false;
            }

            int c_exit_code = WEXITSTATUS(c_result);
            if(c_exit_code == 0) {
                return true;
            } else {
                return false;
            }
        } else {
            std::cout << "An error occurred during the grep command." << std::endl;
            return false;
        }

    }

    bool ContainerController::StopContainer(std::string name) {

        std::string cmd = "docker stop " + name;
        int result = std::system(cmd.c_str());

        if(result == -1) {
            std::cout << "Command execution failed." << std::endl;
            return false;
        }

        int exit_code = WEXITSTATUS(result);
        if(exit_code == 0) {
            return true;
        } else {
            return false;
        }

    }

    bool ContainerController::IsContainerRunning(std::string name) {
        
        std::string cmd = "docker ps --filter name=" + name + " | grep -q " + name;

        int result = std::system(cmd.c_str());

        if(result == 0) {
            std::cout << "ContainerController::IsContainerRunning" << std::endl;
            return true;
        } else if (result == 1) {
            std::cout << name << "is not running." << std::endl;
            return false;
        } else {
            std::cout << "An error occurred during the grep command." << std::endl;
            return false;
        }

    }

}