#ifndef SERVICE_REGISTRY_HPP_
#define SERVICE_REGISTRY_HPP_

#include "lifecycle/UpdateServiceListRequest.hpp"
#include "common/AppUtils.hpp"

#include <string>
#include <vector>
#include <optional>
#include <map>

namespace adsw_manager::agl {

// yaml 값의 `${VAR}` / `${VAR:-default}` 를 환경변수로 치환한다.
//
// 서비스 목록의 정본을 컨테이너 이미지에 두기 위한 장치다 — 보드마다 다른 값
// (호스트 docker gid, 런타임 uid)만 환경변수로 빼면 목록 자체는 모든 보드에서
// 같아진다. AGL 쪽 AppController::RunDocker 는 fork+execvp 로 셸을 거치지 않아
// (ros2 쪽 system() 과 달리) 확장이 공짜가 아니므로 로드 시 여기서 처리한다.
//
// 규칙은 셸과 같다: 미정의 또는 빈 값이면 `:-default`, default 도 없으면 빈 문자열.
// 헤더에 노출한 이유는 단위 테스트 때문이다 — 잘못 확장하면 gid 가 조용히 틀려
// 런처가 docker.sock 접근을 잃는다.
std::string ExpandEnvVars(const std::string& in);

    struct AppDockerInfo {
        std::string image;
        std::vector<std::string> extra_args;
        std::vector<std::string> command_args;

        // 관찰 전용(observe-only) 항목.
        //
        // 왜 필요한가: oss-mmf-ext-agl 같은 **인프라 컨테이너**는 호스트의
        // run 스크립트/compose 가 만들고 소유한다. lfc_monitor 에 상태는
        // 보이게 하고 싶지만(운영자가 코어가 살아있는지 봐야 한다), 그 수명을
        // adsw_manager 가 쥐면 안 된다:
        //   · 자동 복구가 걸리면 우리 yaml 인자로 `docker run` 해 **호스트가
        //     의도한 것과 다른 컨테이너**를 만들 수 있다 (인자 정본은 run 스크립트다)
        //   · ext-agl 은 adsw_manager 자신이 도는 컨테이너다 — stop 은 자기 자신을 죽인다
        // image 는 채워 두되(그래야 컨테이너 카드로 발행되고 docker inspect 로
        // 실상태를 읽는다) 제어 경로만 막는다. domain_agent 와 같은 경계 판단:
        // "상태를 읽을 권한과 서비스를 죽일 권한은 다르다".
        bool observe_only = false;
    };

    class ServiceRegistry {

        public:
            virtual bool Initialize(const std::string& config_path = "");

            virtual std::vector<lifecyclemgr::Service> GetAppList() const;
            virtual std::optional<std::pair<lifecyclemgr::Service, lifecyclemgr::ServiceState>> GetAppInfo(const std::string& app_name) const;
            virtual std::vector<std::pair<std::string, lifecyclemgr::ServiceState>> GetNameStatePairs() const;
            virtual lifecyclemgr::ServiceState GetAppState(const std::string& app_name) const;
            virtual std::optional<AppDockerInfo> GetDockerInfo(const std::string& app_name) const;

            void AddApp(const lifecyclemgr::Service& info);
            void AddApp(const lifecyclemgr::Service& info, lifecyclemgr::ServiceState init_state);
            void RemoveApp(const std::string& app_name);
            virtual void UpdateAppState(const std::string& app_name, lifecyclemgr::ServiceState state);

            virtual ~ServiceRegistry() = default;

        private:
            std::map<lifecyclemgr::Service, lifecyclemgr::ServiceState> app_list_;
            std::map<std::string, AppDockerInfo> docker_info_;

            bool LoadFromYaml(const std::string& path);

    };

} // namespace adsw_manager::agl

#endif
