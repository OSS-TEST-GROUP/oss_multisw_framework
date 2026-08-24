#ifndef ROS2_ADSW_MANAGER_CONTROL_HELPER_UTILS_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_HELPER_UTILS_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <array>
#include <memory>
#include <regex>  // ExecCommand(const std::regex&) 오버로드가 사용 — 자체 include

namespace ros2 {
namespace adsw_manager {

class HelperUtils{

    public:

    // Function to run shell command and return output
    static std::string ExecCommand(const std::string& cmd, const std::string& keyword, const int8_t maxline = 2) {
        std::array<char, 1024> buffer{};
        std::string result;
        int8_t line = 0;
        FILE* pipe = popen(cmd.c_str(), "r");

        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            if (!keyword.empty()){
                result = buffer.data();
                if (result.find(keyword) != std::string::npos){
                    break;
                }
            }else{
                result += buffer.data();
            }
            if (++line > maxline){
                break;
            }
        }

        pclose(pipe);
        return result;
    }
    static std::string ExecCommand(const std::string& cmd, const std::regex& re, const int8_t maxline = 2) {
        std::array<char, 1024> buffer{};
        std::string result;
        std::smatch match;
        int8_t line = 0;
        FILE* pipe = popen(cmd.c_str(), "r");

        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result = buffer.data();
            if (std::regex_search(result, match, re)) {
                break;
            }
            if (++line > maxline){
                break;
            }
        }

        pclose(pipe);
        return result;
    }

    // Split string into vector of lines
    static std::vector<std::string> SplitLines(const std::string& input) {
        std::istringstream ss(input);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }
};

} // namespace adsw_manager
} // namespace ros2

#endif
