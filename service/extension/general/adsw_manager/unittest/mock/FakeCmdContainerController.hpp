#ifndef GENERAL_ADSW_MANAGER_UNITTEST_MOCK_FAKE_CMD_CONTAINER_CONTROLLER_HPP_
#define GENERAL_ADSW_MANAGER_UNITTEST_MOCK_FAKE_CMD_CONTAINER_CONTROLLER_HPP_

#include <initializer_list>
#include <map>
#include <string>
#include <vector>

#include "control/ContainerController.hpp"

namespace general {
namespace adsw_manager {
namespace testing_support {

// ContainerController 의 외부 프로세스 실행 seam 을 가로채는 fake.
//
// 실제 구현은 system()/popen() 으로 docker · kubectl 을 부른다. 그 경로가
// 유닛 레인에서 통째로 미커버였던 이유이고, 여기서 가로채면 docker 없이
// 같은 로직을 전부 돌릴 수 있다.
//
//   1) 명령 문자열 검증 — run_cmds / capture_cmds 를 그대로 노출
//   2) 협력 객체로 쓰기 — 컨테이너별 status/exit-code 를 미리 심어두고
//      IsContainerRunning() / GetContainerExitCode() 가 그걸 반환하게 함
class FakeCmdContainerController : public ContainerController {
  public:
    std::vector<std::string> run_cmds;
    std::vector<std::string> capture_cmds;

    int              default_rc = 0;
    std::vector<int> rc_queue;

    std::string capture_out;
    std::map<std::string, std::string> capture_by_substr;

    void PushRc(std::initializer_list<int> rcs) { rc_queue.insert(rc_queue.end(), rcs); }

    void SetContainerRunning(const std::string& name, bool running, int32_t exit_code = 0) {
        capture_by_substr["{{.State.Status}}' " + name]   = running ? "running\n" : "exited\n";
        capture_by_substr["{{.State.ExitCode}}' " + name]  = std::to_string(exit_code) + "\n";
    }

  protected:
    int RunCommand(const std::string& cmd) override {
        run_cmds.push_back(cmd);
        if (rc_queue.empty()) return default_rc;
        int rc = rc_queue.front();
        rc_queue.erase(rc_queue.begin());
        return rc;
    }

    std::string CaptureCommand(const std::string& cmd, size_t max_len) override {
        capture_cmds.push_back(cmd);
        std::string out = capture_out;
        for (const auto& kv : capture_by_substr) {
            if (cmd.find(kv.first) != std::string::npos) { out = kv.second; break; }
        }
        // 실제 구현은 fgets 로 max_len-1 바이트까지만 읽는다. 그 절단도 재현한다.
        return out.substr(0, max_len ? max_len - 1 : 0);
    }
};

} // namespace testing_support
} // namespace adsw_manager
} // namespace general

#endif
