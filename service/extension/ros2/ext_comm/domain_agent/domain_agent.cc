#include "domain_agent.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicData.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp>
#include <fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>

using namespace eprosima::fastdds::dds;

namespace {

// 이 데몬이 쓰는 두 토픽 (ROS 이름 → rmw 규약상 `rt/` 접두사가 붙는다).
constexpr const char* kSetTopic   = "rt/mmf/domain/set";
constexpr const char* kStateTopic = "rt/mmf/domain/state";
constexpr const char* kStringType = "std_msgs::msg::dds_::String_";

// 앞뒤 공백 제거.
std::string Trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// "0"~"232" 만 통과. 부호·공백·빈 문자열·초과값은 모두 거부한다.
// std::atoi 를 쓰지 않는 이유: "abc" 를 0 으로 조용히 읽어 도메인 0 에 붙는 사고를
// 만든다(도메인 0 은 유효한 값이라 오류로 드러나지도 않는다).
bool ParseDomainValue(const std::string& raw, int* out) {
    const std::string t = Trim(raw);
    if (t.empty() || t.size() > 3) return false;
    for (char c : t) {
        if (c < '0' || c > '9') return false;
    }
    const int v = std::stoi(t);
    if (v < kMinDomainId || v > kMaxDomainId) return false;
    *out = v;
    return true;
}

}  // namespace

DomainRequest ParseDomainRequest(const std::string& payload) {
    DomainRequest req;
    bool have_dds = false, have_ros = false;

    // 줄 단위로 자르고(주석은 줄 끝까지), 각 줄을 다시 `;`·공백으로 자른다.
    // 개행만 허용하면 클라이언트마다 깨진다 — 실측: `ros2 topic pub` 으로 보낸
    // 개행이 공백으로 접혀 "DDS_DOMAIN_ID=5 ROS_DOMAIN_ID=42" 한 줄이 되어 거부됐다.
    // 포맷은 여전히 KEY=VALUE 쌍이고, 구분자만 관대하게 받는다.
    std::istringstream lines(payload);
    std::string line;
    while (std::getline(lines, line)) {
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        for (char& c : line) {
            if (c == ';' || c == ',' || c == '\t' || c == '\r') c = ' ';
        }
        std::istringstream toks(line);
        std::string tok;
        while (toks >> tok) {
            const auto eq = tok.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = Trim(tok.substr(0, eq));
            const std::string val = tok.substr(eq + 1);
            if (key == "DDS_DOMAIN_ID") {
                if (!ParseDomainValue(val, &req.dds)) {
                    req.error = "DDS_DOMAIN_ID 값이 0~232 정수가 아님";
                    return req;
                }
                have_dds = true;
            } else if (key == "ROS_DOMAIN_ID") {
                if (!ParseDomainValue(val, &req.ros)) {
                    req.error = "ROS_DOMAIN_ID 값이 0~232 정수가 아님";
                    return req;
                }
                have_ros = true;
            }
        }
    }
    if (!have_dds || !have_ros) {
        req.error = "DDS_DOMAIN_ID / ROS_DOMAIN_ID 두 키가 모두 필요함(부분 갱신 미지원)";
        return req;
    }
    req.ok = true;
    return req;
}

std::string RenderDomainEnv(int dds, int ros) {
    std::ostringstream out;
    out << "# hmi_launcher Settings 가 관리하는 MMF 도메인 (두 평면).\n"
        << "# 이 파일은 domain_agent 가 원격(HMI 보드)의 요청을 받아 갱신한다.\n"
        << "# 변경은 각 서비스 재기동(컨테이너 재생성) 시 반영된다.\n"
        << "#\n"
        << "# DDS_DOMAIN_ID  : 순수 Fast-DDS 버스 — ota/lifecycle/diagnostics/adsw.\n"
        << "# ROS_DOMAIN_ID  : ROS2(rmw) — rviz2/Autoware/rosbridge.\n"
        << "DDS_DOMAIN_ID=" << dds << "\n"
        << "ROS_DOMAIN_ID=" << ros << "\n";
    return out.str();
}

bool WriteFileAtomic(const std::string& path, const std::string& body, std::string* err) {
    const std::string tmp = path + ".tmp." + std::to_string(::getpid());
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            if (err) *err = "임시 파일 생성 실패: " + tmp;
            return false;
        }
        out << body;
        if (!out) {
            if (err) *err = "임시 파일 쓰기 실패: " + tmp;
            return false;
        }
    }
    // rename 은 같은 파일시스템에서 원자적이다 — 읽는 쪽이 반쯤 쓰인 파일을 볼 일이 없다.
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        if (err) *err = "rename 실패: " + tmp + " → " + path;
        std::remove(tmp.c_str());
        return false;
    }
    return true;
}

// 파일명에 셸 특수문자가 없어야 한다 — 아래 docker 명령 문자열에 들어가기 때문이다.
// 값은 env(MMF_DOMAIN_HOST_ENV_FILE)에서 오므로 신뢰하지만, 확인은 공짜다.
static bool IsShellSafePath(const std::string& p) {
    for (char c : p) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '/' || c == '.' ||
                        c == '_' || c == '-';
        if (!ok) return false;
    }
    return !p.empty();
}

// 이미지 이름은 경로 문자 + ':'(태그 구분자)까지 허용한다.
static bool IsShellSafeImage(const std::string& img) {
    if (img.empty()) return false;
    for (char c : img) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '/' || c == '.' ||
                        c == '_' || c == '-' || c == ':';
        if (!ok) return false;
    }
    return true;
}

std::string DiscoverSelfImage() {
    const char* n = std::getenv("MMF_SELF_CONTAINER");
    std::string cname = (n && *n) ? n : "oss-mmf-ext-ros2";
    if (!IsShellSafePath(cname)) return "";
    const std::string cmd =
        "docker inspect -f '{{.Config.Image}}' " + cname + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[512] = {0};
    std::string out;
    if (fgets(buf, sizeof(buf), pipe)) out = Trim(buf);
    pclose(pipe);
    return out;
}

bool WriteFileViaDocker(const std::string& host_path, const std::string& body,
                        const std::string& image, std::string* err) {
    if (!IsShellSafePath(host_path)) {
        if (err) *err = "호스트 경로에 셸 특수문자: " + host_path;
        return false;
    }
    if (!IsShellSafeImage(image)) {
        if (err) *err = "헬퍼 이미지를 찾지 못했거나 이름이 부적절: '" + image + "'";
        return false;
    }
    const auto slash = host_path.rfind('/');
    if (slash == std::string::npos || slash == 0) {
        if (err) *err = "호스트 경로가 절대경로가 아님: " + host_path;
        return false;
    }
    const std::string dir  = host_path.substr(0, slash);
    const std::string base = host_path.substr(slash + 1);

    // 본문은 stdin 으로만 넘긴다. 컨테이너 안에서 tmp 로 쓴 뒤 mv → 원자적 교체.
    // --user 0:0 이 필요하다: 호스트 도메인 디렉토리는 root 소유이고(실측 2026-09-04
    // /opt/mmf/config = root:root 755) 이미지 기본 사용자는 ossuser(1000) 라 그냥 띄우면
    // "cannot create /t/...: Permission denied" 로 실패한다. docker.sock 을 쓸 수 있는
    // 시점에 이미 호스트 root 급 권한이므로 새로 얻는 권한은 없다.
    const std::string cmd =
        "docker run --rm -i --user 0:0 -v " + dir + ":/t --entrypoint sh " + image +
        " -c 'cat > /t/" + base + ".tmp && mv /t/" + base + ".tmp /t/" + base + "'"
        " >/dev/null 2>&1";
    FILE* pipe = popen(cmd.c_str(), "w");
    if (!pipe) {
        if (err) *err = "docker 헬퍼 실행 실패(popen)";
        return false;
    }
    fwrite(body.data(), 1, body.size(), pipe);
    const int rc = pclose(pipe);
    if (rc != 0) {
        if (err) *err = "docker 헬퍼가 0 이 아닌 코드로 종료(rc=" + std::to_string(rc) + ")";
        return false;
    }
    return true;
}

// 호스트 파일을 docker.sock 경유로 읽는다. 마운트가 없는 배포에서 상태 토픽의
// file_* 가 '?' 로만 나오면 "반영됐는지" 를 알 수 없어 관측 목적을 잃는다.
std::string ReadFileViaDocker(const std::string& host_path, const std::string& image) {
    if (!IsShellSafePath(host_path) || !IsShellSafeImage(image)) return "";
    const auto slash = host_path.rfind('/');
    if (slash == std::string::npos || slash == 0) return "";
    const std::string dir  = host_path.substr(0, slash);
    const std::string base = host_path.substr(slash + 1);
    const std::string cmd =
        "docker run --rm --user 0:0 -v " + dir + ":/t:ro --entrypoint sh " + image +
        " -c 'cat /t/" + base + " 2>/dev/null'";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    std::string out;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) out += buf;
    pclose(pipe);
    return out;
}

// ── DomainSetListener ──────────────────────────────────────────────────

DomainSetListener::DomainSetListener(DynamicType::_ref_type type, DomainAgent* agent)
    : type_(type), agent_(agent) {}

void DomainSetListener::on_data_available(DataReader* reader) {
    DynamicData::_ref_type data = DynamicDataFactory::get_instance()->create_data(type_);
    SampleInfo info;
    if (reader->take_next_sample(&data, &info) != RETCODE_OK) return;
    if (!info.valid_data) return;

    std::string payload;
    if (data->get_string_value(payload, data->get_member_id_by_name("data")) != RETCODE_OK) {
        std::cerr << "[domain_agent] String_ 의 data 멤버를 읽지 못했다\n";
        return;
    }
    if (agent_) agent_->HandleRequest(payload);
}

// ── DomainAgent ───────────────────────────────────────────────────────

bool DomainAgent::Initialize(const std::string& idl_root, const std::string& env_path) {
    env_path_ = env_path;
    auto* factory = DomainParticipantFactory::get_instance();

    // ROS 평면에 붙는다 — 명령이 rosbridge(rmw, ROS_DOMAIN_ID)를 통해 오기 때문이다.
    int ros_domain = 0;
    if (const char* r = std::getenv("ROS_DOMAIN_ID")) {
        int v = 0;
        if (ParseDomainValue(r, &v)) ros_domain = v;
    }
    participant_ = factory->create_participant(ros_domain, PARTICIPANT_QOS_DEFAULT);
    if (!participant_) {
        std::cerr << "[domain_agent] create_participant 실패 (ros domain " << ros_domain << ")\n";
        return false;
    }
    publisher_  = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!publisher_ || !subscriber_) {
        std::cerr << "[domain_agent] publisher/subscriber 생성 실패\n";
        return false;
    }

    auto builder = DynamicTypeBuilderFactory::get_instance()
        ->create_type_w_uri(idl_root + "/std_msgs/String.idl", "String_", {idl_root});
    if (!builder) {
        std::cerr << "[domain_agent] std_msgs/String.idl 파싱 실패 (idl_root=" << idl_root << ")\n";
        return false;
    }
    string_type_ = builder->build();

    // dds_bridge 와 같은 이유로 이름별 TypeSupport 를 따로 등록한다.
    TypeSupport ts_set(new DynamicPubSubType(string_type_));
    ts_set.register_type(participant_, kStringType);

    Topic* set_topic = participant_->create_topic(kSetTopic, kStringType, TOPIC_QOS_DEFAULT);
    Topic* state_topic = participant_->create_topic(kStateTopic, kStringType, TOPIC_QOS_DEFAULT);
    if (!set_topic || !state_topic) {
        std::cerr << "[domain_agent] create_topic 실패\n";
        return false;
    }

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // rosbridge 가 보낸 마지막 요청 하나는 늦게 뜬 agent 도 받아야 한다.
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 1;
    // 동적 타입의 TypeObject 는 rosbridge 쪽 IDL 컴파일 결과와 바이트 일치하지 않는다
    // (dds_bridge 와 같은 사유) — CDR 레이아웃만 같으면 되므로 강제 검증을 끈다.
    rqos.type_consistency().m_kind = ALLOW_TYPE_COERCION;
    rqos.type_consistency().m_force_type_validation = false;
    rqos.type_consistency().m_ignore_member_names = true;
    rqos.type_consistency().m_ignore_string_bounds = true;
    rqos.type_consistency().m_ignore_sequence_bounds = true;

    listener_ = std::make_unique<DomainSetListener>(string_type_, this);
    if (!subscriber_->create_datareader(set_topic, rqos, listener_.get())) {
        std::cerr << "[domain_agent] create_datareader 실패\n";
        return false;
    }

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    // 마지막 상태를 latch — 늦게 붙는 런처가 즉시 현재 상태를 본다.
    wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 1;
    state_writer_ = publisher_->create_datawriter(state_topic, wqos);
    if (!state_writer_) {
        std::cerr << "[domain_agent] state writer 생성 실패\n";
        return false;
    }

    std::cout << "[domain_agent] ros domain=" << ros_domain
              << " env=" << env_path_ << "\n"
              << "[domain_agent] listening " << kSetTopic
              << ", publishing " << kStateTopic << "\n";
    PublishState("started");
    return true;
}

void DomainAgent::HandleRequest(const std::string& payload) {
    const DomainRequest req = ParseDomainRequest(payload);
    if (!req.ok) {
        std::cerr << "[domain_agent] 요청 거부: " << req.error << "\n";
        PublishState("rejected: " + req.error);
        return;
    }

    // 받은 문자열을 그대로 쓰지 않는다 — 값만 취해 정규 형식으로 렌더한다.
    const std::string body = RenderDomainEnv(req.dds, req.ros);

    // ① 컨테이너 안에서 직접 쓸 수 있으면 그렇게 한다(마운트가 있는 배포).
    std::string err;
    if (WriteFileAtomic(env_path_, body, &err)) {
        std::cout << "[domain_agent] 반영(direct): DDS=" << req.dds
                  << " ROS=" << req.ros << " → " << env_path_ << " (재생성 필요)\n";
        PublishState("applied(direct): needs container recreate");
        return;
    }

    // ② 마운트가 없으면 docker.sock 경유로 호스트에 쓴다. 배포 요구가 "컨테이너만
    //    업데이트하면 동작" 이라 새 마운트를 전제할 수 없다(설계 근거는 헤더 참조).
    const char* h = std::getenv("MMF_DOMAIN_HOST_ENV_FILE");
    const std::string host_path = (h && *h) ? h : "/opt/mmf/config/domain.env";
    const std::string image = DiscoverSelfImage();
    std::string derr;
    if (WriteFileViaDocker(host_path, body, image, &derr)) {
        std::cout << "[domain_agent] 반영(docker): DDS=" << req.dds
                  << " ROS=" << req.ros << " → " << host_path << " (재생성 필요)\n";
        PublishState("applied(docker): needs container recreate");
        return;
    }
    std::cerr << "[domain_agent] 도메인 파일 쓰기 실패 — direct: " << err
              << " / docker: " << derr << "\n";
    PublishState("write-failed: direct=" + err + "; docker=" + derr);
}

void DomainAgent::PublishState(const std::string& note) {
    if (!state_writer_) return;

    // 현재 파일에 무엇이 적혀 있는지 (요청 반영 여부의 근거).
    std::string file_dds = "?", file_ros = "?";
    {
        std::string content;
        std::ifstream in(env_path_);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            content = ss.str();
        } else {
            // 마운트가 없는 배포 — 쓰기와 같은 통로로 읽어 온다.
            const char* h = std::getenv("MMF_DOMAIN_HOST_ENV_FILE");
            const std::string host_path = (h && *h) ? h : "/opt/mmf/config/domain.env";
            content = ReadFileViaDocker(host_path, DiscoverSelfImage());
        }
        const DomainRequest cur = ParseDomainRequest(content);
        if (cur.ok) {
            file_dds = std::to_string(cur.dds);
            file_ros = std::to_string(cur.ros);
        }
    }
    // 이 프로세스가 실제로 도는 도메인 (파일과 다르면 = 재생성 아직 안 됨).
    const char* run_ros = std::getenv("ROS_DOMAIN_ID");
    const char* run_dds = std::getenv("MMF_DDS_DOMAIN_ID");
    if (!run_dds) run_dds = std::getenv("DDS_DOMAIN_ID");

    std::ostringstream body;
    body << "file_dds=" << file_dds << ";file_ros=" << file_ros
         << ";running_dds=" << (run_dds ? run_dds : "?")
         << ";running_ros=" << (run_ros ? run_ros : "?")
         << ";note=" << note;

    DynamicData::_ref_type data = DynamicDataFactory::get_instance()->create_data(string_type_);
    if (data->set_string_value(data->get_member_id_by_name("data"), body.str()) != RETCODE_OK) {
        std::cerr << "[domain_agent] 상태 페이로드 설정 실패\n";
        return;
    }
    if (state_writer_->write(&data) != RETCODE_OK) {
        std::cerr << "[domain_agent] 상태 발행 실패\n";
    }
}
