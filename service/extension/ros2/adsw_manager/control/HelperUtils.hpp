
#ifndef ROS2_ADSW_MANAGER_CONTROL_HELPER_UTILS_HPP_
#define ROS2_ADSW_MANAGER_CONTROL_HELPER_UTILS_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <array>
#include <memory>

namespace ros2 {
namespace adsw_manager {

class HelperUtils{

    public:

    // Function to run shell command and return output
    static std::string ExecCommand(const std::string& cmd) {
        std::array<char, 128> buffer{};
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");

        if (!pipe)
            throw std::runtime_error("popen() failed!");

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
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
