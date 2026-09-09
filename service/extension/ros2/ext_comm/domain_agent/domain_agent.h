// domain_agent — 다른 호스트의 HMI Settings 가 정한 도메인을 이 호스트의
// 도메인 파일에 반영하는 데몬.
//
// 왜 필요한가
// ------------
// hmi_launcher 는 **자기가 도는 호스트의** `domain.env` 만 쓴다. 그런데 Autoware /
// MMF core / ext-ros2 는 다른 호스트(차량 컴퓨트)에서 돌기 때문에, HMI 보드에서
// 도메인을 바꿔도 그쪽 호스트는 옛 값을 그대로 쓴다. 실측 2026-09-04: 81 보드
// Settings 를 5/5 로 저장했으나 151 보드는 42/42 로 남았다.
//
// 이 데몬이 그 경로를 잇는다:
//
//   hmi_launcher(81) --ws--> rosbridge(151) --ROS 평면--> domain_agent(151)
//                                                          → /opt/mmf/config/domain.env
//
// 왜 ROS 평면인가: 명령이 오는 길이 rosbridge 이고, rosbridge 는 rmw 가
// `ROS_DOMAIN_ID` 에 올린다. 또 이 명령은 **도메인을 바꾸기 전** 도메인으로 도착해야
// 하므로(바꾼 뒤엔 서로 안 보인다) 현재 ROS 평면에 붙는 것이 맞다.
//
// 하지 않는 일 (경계)
// -------------------
// 컨테이너를 재기동하지 않는다. 도메인은 프로세스 시작 시에만 읽히므로 반영에는
// 재생성이 필요하지만, 설정 파일을 쓰는 권한과 서비스를 죽이는 권한은 다르다.
// 컨테이너 제어는 lifecycle_mgr / adsw_manager 의 책임이고, 사용자에게는 런처가
// "재생성 필요" 를 알린다(MmfRestarter). 이 데몬은 **사실을 파일에 남기고 상태를
// 되돌려 줄 뿐**이다.
#pragma once

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicType.hpp>

#include <string>

// 도메인 ID 유효 범위 (ROS 2 / Fast-DDS 공통).
constexpr int kMinDomainId = 0;
constexpr int kMaxDomainId = 232;

// 파싱 결과. ok=false 면 payload 를 신뢰할 수 없다 — 파일을 건드리지 않는다.
struct DomainRequest {
    bool ok = false;
    int dds = -1;
    int ros = -1;
    std::string error;
};

// `KEY=VALUE` 줄들에서 DDS_DOMAIN_ID / ROS_DOMAIN_ID 를 뽑는다.
//
// 와이어 포맷을 도메인 파일과 **같은 형식**으로 둔 이유: 런처가 자기 호스트에 쓰는
// 내용을 그대로 보내면 되고, 사람이 로그만 봐도 무엇이 오갔는지 안다. 단 받은
// 문자열을 파일에 그대로 쓰지는 않는다 — 값만 취해 정규 형식으로 다시 쓴다
// (원격이 보낸 임의 텍스트가 이 호스트의 설정 파일이 되면 안 된다).
//
// 두 키가 모두 있어야 ok=true. 하나만 바꾸는 부분 갱신은 지원하지 않는다 —
// 절반만 적용된 도메인 상태를 만들지 않기 위한 의도적 제약이다.
DomainRequest ParseDomainRequest(const std::string& payload);

// 도메인 파일 본문을 만든다 (hmi_launcher 가 쓰는 것과 같은 형식).
std::string RenderDomainEnv(int dds, int ros);

// [path] 에 원자적으로 쓴다: 같은 디렉토리 임시파일 → rename.
// 읽는 쪽(run 스크립트/compose)이 반쯤 쓰인 파일을 보는 일이 없어야 한다.
bool WriteFileAtomic(const std::string& path, const std::string& body, std::string* err);

// 호스트 경로에 docker.sock 을 경유해 쓴다.
//
// 왜 이 경로가 필요한가: 배포 요구가 **"컨테이너만 업데이트하면 동작"** 이다. 그런데
// 도메인 파일은 호스트에 있고, ext-ros2 에는 그 디렉토리 마운트가 없다(실측 2026-09-04:
// /dev/shm, docker.sock, ${SAVED_REGISTRY_BACKUP:-/dev/null}:/config 뿐). 새 마운트를
// 요구하면 호스트 compose/run 스크립트를 함께 고쳐야 하므로 요구를 못 맞춘다.
//
// docker.sock 은 이미 마운트돼 있다 — adsw_manager 가 호스트 컨테이너를 만들기 위해
// 그렇게 설계됐다. 같은 통로로 짧게 뜨는 컨테이너에 호스트 디렉토리를 붙여 쓴다.
// 본문은 **stdin 으로** 넘긴다(명령 문자열에 넣지 않는다) → payload 로 인한 셸 주입 없음.
//
// [image] 는 헬퍼로 쓸 이미지. 자기 이미지를 쓰면 보드에 반드시 존재한다.
bool WriteFileViaDocker(const std::string& host_path, const std::string& body,
                        const std::string& image, std::string* err);

// 호스트 파일을 docker.sock 경유로 읽는다 (마운트 없는 배포의 상태 관측용).
std::string ReadFileViaDocker(const std::string& host_path, const std::string& image);

// 잘 알려진 컨테이너 이름으로 자기 이미지를 찾는다 (MMF_SELF_CONTAINER, 기본
// oss-mmf-ext-ros2). 실패하면 빈 문자열 — 호출부가 이유를 상태로 알린다.
std::string DiscoverSelfImage();

class DomainAgent;

// 수신 리스너 — 샘플 하나를 받아 DomainAgent 에 넘긴다.
class DomainSetListener : public eprosima::fastdds::dds::DataReaderListener {
  public:
    DomainSetListener(eprosima::fastdds::dds::DynamicType::_ref_type type, DomainAgent* agent);
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;

  private:
    eprosima::fastdds::dds::DynamicType::_ref_type type_;
    DomainAgent* agent_;
};

// ROS 평면에 붙어 도메인 변경 요청을 받고, 결과 상태를 되돌려 발행한다.
class DomainAgent {
  public:
    // [idl_root] 는 std_msgs/String.idl 을 찾을 루트, [env_path] 는 쓸 도메인 파일.
    bool Initialize(const std::string& idl_root, const std::string& env_path);

    // payload 를 검증하고 도메인 파일에 반영한다. 성공/실패 모두 상태를 발행한다.
    void HandleRequest(const std::string& payload);

    // 현재 파일 내용 + 이 프로세스가 보는 도메인을 상태 토픽에 발행한다.
    void PublishState(const std::string& note);

  private:
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher*         publisher_   = nullptr;
    eprosima::fastdds::dds::Subscriber*        subscriber_  = nullptr;
    eprosima::fastdds::dds::DataWriter*        state_writer_ = nullptr;
    eprosima::fastdds::dds::DynamicType::_ref_type string_type_;
    std::unique_ptr<DomainSetListener> listener_;
    std::string env_path_;
};
