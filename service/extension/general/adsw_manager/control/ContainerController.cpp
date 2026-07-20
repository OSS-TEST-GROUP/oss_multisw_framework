#include "ContainerController.hpp"
#include "logger/Log.hpp"

namespace ros2 {
namespace adsw_manager {

const std::string ADSW_ROS2_CONTAINER_NAME = "oss-mmf-ext-ros2";

ContainerController::ContainerController() {
    LOG_DEBUG("ContainerController create");
}

ContainerController::~ContainerController() {}

bool ContainerController::Initialize() {
    return true;
}

void ContainerController::Finalize() {}

// ─────────────────────────────────────────────────────────────
// Public interface: dispatch to per-type implementation
// ─────────────────────────────────────────────────────────────

bool ContainerController::StartContainer(const std::string& container_name) {
    LOG_DEBUG("ContainerController::StartContainer " << container_name);
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return false;

    switch (container->GetContainerType()) {
        case ContainerType::K3S:            return StartK3sContainer(container);
        case ContainerType::DOCKER:         return StartDockerContainer(container);
        case ContainerType::DOCKER_COMPOSE: return StartDockerComposeContainer(container);
        default:
            LOG_ERROR("ContainerController::StartContainer unknown type: " << container_name);
            return false;
    }
}

bool ContainerController::StopContainer(const std::string& container_name) {
    LOG_DEBUG("ContainerController::StopContainer " << container_name);
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return false;

    switch (container->GetContainerType()) {
        case ContainerType::K3S:            return StopK3sContainer(container);
        case ContainerType::DOCKER:         return StopDockerContainer(container);
        case ContainerType::DOCKER_COMPOSE: return StopDockerComposeContainer(container);
        default:
            LOG_ERROR("ContainerController::StopContainer unknown type: " << container_name);
            return false;
    }
}

bool ContainerController::RestartContainer(const std::string& container_name) {
    LOG_DEBUG("ContainerController::RestartContainer " << container_name);
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return false;

    switch (container->GetContainerType()) {
        case ContainerType::K3S:            return RestartK3sContainer(container);
        case ContainerType::DOCKER:         return RestartDockerContainer(container);
        case ContainerType::DOCKER_COMPOSE: return RestartDockerComposeContainer(container);
        default:
            LOG_ERROR("ContainerController::RestartContainer unknown type: " << container_name);
            return false;
    }
}

bool ContainerController::GetContainerStatus(const std::string& container_name) {
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return false;

    std::string cmd;
    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            cmd = "kubectl get pod " + container->GetPodName()
                + " -n " + container->GetNamespace()
                + " -o jsonpath='{.status.phase}'";
            break;
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            cmd = "docker inspect --format='{{.State.Status}}' " + container_name;
            break;
        default:
            return false;
    }
    return RunCommand(cmd) == 0;
}

bool ContainerController::GetContainerLogs(const std::string& container_name) {
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return false;

    std::string cmd;
    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            cmd = "kubectl logs " + container->GetPodName()
                + " -n " + container->GetNamespace()
                + " -c " + container_name;
            break;
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            cmd = "docker logs " + container_name;
            break;
        default:
            return false;
    }
    return RunCommand(cmd) == 0;
}

// ─────────────────────────────────────────────────────────────
// k3s (kubectl) implementations
// ─────────────────────────────────────────────────────────────

bool ContainerController::StartK3sContainer(std::shared_ptr<Container> container) {
    std::string cmd = "kubectl apply -f " + container->GetDeployFile();
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::StartK3sContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StartK3sContainer failed " << container->GetName());
    return false;
}

bool ContainerController::StopK3sContainer(std::shared_ptr<Container> container) {
    std::string cmd = "kubectl delete pod " + container->GetName()
                    + " -n " + container->GetNamespace() + " --ignore-not-found";
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::StopK3sContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StopK3sContainer failed " << container->GetName());
    return false;
}

bool ContainerController::RestartK3sContainer(std::shared_ptr<Container> container) {
    // k3s pods are immutable: delete + re-apply
    if (!StopK3sContainer(container)) return false;
    return StartK3sContainer(container);
}

// ─────────────────────────────────────────────────────────────
// docker run/stop implementations
// ─────────────────────────────────────────────────────────────

bool ContainerController::StartDockerContainer(std::shared_ptr<Container> container) {
    // Try to start existing stopped container first, otherwise run a new one
    std::string start_cmd = "docker start " + container->GetName() + " 2>/dev/null";
    if (RunCommand(start_cmd) == 0) {
        LOG_DEBUG("ContainerController::StartDockerContainer restarted existing " << container->GetName());
        return true;
    }

    // `--init` injects docker-init (tini) as PID 1, which reap()s child
    // processes. Without it, services we `docker exec ros2 run`'d into the
    // container become zombies when they exit, because the container's own
    // entrypoint (`tail -f /dev/null` in adsw-test-app) never calls wait().
    // Zombies retain their cmdline only briefly, so a poll cycle later
    // IsServiceAliveInContainer's `pgrep -f` misses them and reports alive=no
    // — which under PolicyAlways/OnFailure triggers an auto-restart, which
    // produces another zombie, and the operator sees an unending Stop/Start
    // loop. tini fixes the root cause: dead services stay reaped, alive
    // services stay matched.
    // docker run -d --init --name <n> <extra_args...> <image> <command_args...>
    // extra_args (--network host, -e, -v ...) and command_args (the container's
    // own CMD, e.g. an Autoware role start script) come from the YAML container
    // spec — see Container::SetExtraArgs/SetCommandArgs. Empty for the classic
    // adsw-test-app model, which keeps the original bare-run behaviour.
    std::string run_cmd = "docker run -d --init --name " + container->GetName();
    for (const auto& arg : container->GetExtraArgs()) {
        run_cmd += " " + arg;
    }
    run_cmd += " " + container->GetImage();
    for (const auto& carg : container->GetCommandArgs()) {
        run_cmd += " " + carg;
    }
    if (RunCommand(run_cmd) == 0) {
        LOG_DEBUG("ContainerController::StartDockerContainer new container " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StartDockerContainer failed " << container->GetName());
    return false;
}

bool ContainerController::StopDockerContainer(std::shared_ptr<Container> container) {
    std::string cmd = "docker stop " + container->GetName()
                    + " && docker rm " + container->GetName();
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::StopDockerContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StopDockerContainer failed " << container->GetName());
    return false;
}

bool ContainerController::RestartDockerContainer(std::shared_ptr<Container> container) {
    std::string cmd = "docker restart " + container->GetName();
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::RestartDockerContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::RestartDockerContainer failed " << container->GetName());
    return false;
}

// ─────────────────────────────────────────────────────────────
// docker compose implementations
// ─────────────────────────────────────────────────────────────

bool ContainerController::StartDockerComposeContainer(std::shared_ptr<Container> container) {
    std::string cmd = "docker compose -f " + container->GetDeployFile() + " up -d";
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::StartDockerComposeContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StartDockerComposeContainer failed " << container->GetName());
    return false;
}

bool ContainerController::StopDockerComposeContainer(std::shared_ptr<Container> container) {
    std::string cmd = "docker compose -f " + container->GetDeployFile() + " down";
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::StopDockerComposeContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::StopDockerComposeContainer failed " << container->GetName());
    return false;
}

bool ContainerController::RestartDockerComposeContainer(std::shared_ptr<Container> container) {
    std::string cmd = "docker compose -f " + container->GetDeployFile() + " restart";
    if (RunCommand(cmd) == 0) {
        LOG_DEBUG("ContainerController::RestartDockerComposeContainer success " << container->GetName());
        return true;
    }
    LOG_ERROR("ContainerController::RestartDockerComposeContainer failed " << container->GetName());
    return false;
}

// ─────────────────────────────────────────────────────────────
// BaseCommand helpers (for exec into container)
// ─────────────────────────────────────────────────────────────

std::string ContainerController::BaseCommand(const std::string& container_name) {
    if (container_name == ADSW_ROS2_CONTAINER_NAME) return "";

    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return "";

    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            return "kubectl exec -n " + container->GetNamespace()
                 + " " + container->GetPodName() + " -- ";
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            return "docker exec " + container_name + " ";
        default:
            return "";
    }
}

std::string ContainerController::BaseRos2Command(const std::string& container_name) {
    if (container_name == ADSW_ROS2_CONTAINER_NAME) return "";

    std::string base = BaseCommand(container_name);
    if (base.empty()) return "";

    std::shared_ptr<Container> container = GetContainer(container_name);
    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            return base + "bash -c \"source /opt/ros/humble/setup.bash && ";
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            return base + "bash -c \"source /opt/ros/humble/setup.bash && ";
        default:
            return "";
    }
}

// ─────────────────────────────────────────────────────────────
// Runtime status queries
// ─────────────────────────────────────────────────────────────

std::string ContainerController::GetContainerStatusStr(const std::string& container_name) {
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return "unknown";

    std::string cmd;
    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            cmd = "kubectl get pod " + container->GetPodName()
                + " -n " + container->GetNamespace()
                + " -o jsonpath='{.status.phase}' 2>/dev/null";
            break;
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            cmd = "docker inspect --format='{{.State.Status}}' " + container_name + " 2>/dev/null";
            break;
        default:
            return "unknown";
    }

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "unknown";
    char buf[64] = {};
    fgets(buf, sizeof(buf), pipe);
    pclose(pipe);

    std::string result(buf);
    // strip trailing newline and surrounding single-quotes added by shell
    while (!result.empty() && (result.back() == '\n' || result.back() == '\'' || result.back() == '\r'))
        result.pop_back();
    if (!result.empty() && result.front() == '\'')
        result.erase(0, 1);
    return result.empty() ? "unknown" : result;
}

int32_t ContainerController::GetContainerExitCode(const std::string& container_name) {
    std::shared_ptr<Container> container = GetContainer(container_name);
    if (!container) return -1;

    std::string cmd;
    switch (container->GetContainerType()) {
        case ContainerType::K3S:
            cmd = "kubectl get pod " + container->GetPodName()
                + " -n " + container->GetNamespace()
                + " -o jsonpath='{.status.containerStatuses[0].state.terminated.exitCode}' 2>/dev/null";
            break;
        case ContainerType::DOCKER:
        case ContainerType::DOCKER_COMPOSE:
            cmd = "docker inspect --format='{{.State.ExitCode}}' " + container_name + " 2>/dev/null";
            break;
        default:
            return -1;
    }

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return -1;
    char buf[32] = {};
    fgets(buf, sizeof(buf), pipe);
    pclose(pipe);

    try {
        return static_cast<int32_t>(std::stoi(std::string(buf)));
    } catch (...) {
        return -1;
    }
}

bool ContainerController::IsContainerRunning(const std::string& container_name) {
    std::string status = GetContainerStatusStr(container_name);
    return (status == "running" || status == "Running");
}

std::vector<std::shared_ptr<Container>> ContainerController::GetAllContainers() {
    return containers_;
}

// ─────────────────────────────────────────────────────────────
// Container registry management
// ─────────────────────────────────────────────────────────────

std::shared_ptr<Container> ContainerController::GetContainer(const std::string& container_name) {
    for (const auto& container : containers_) {
        if (container->GetName() == container_name) {
            return container;
        }
    }
    LOG_ERROR("ContainerController::GetContainer not found: " << container_name);
    return nullptr;
}

bool ContainerController::AddContainer(std::shared_ptr<Container> container) {
    if (!container) {
        LOG_ERROR("ContainerController::AddContainer received null container.");
        return false;
    }
    for (const auto& existing : containers_) {
        if (existing->GetName() == container->GetName()) {
            LOG_ERROR("ContainerController::AddContainer already exists: " << container->GetName());
            return false;
        }
    }
    containers_.push_back(container);
    LOG_DEBUG("ContainerController::AddContainer added: " << container->GetName()
              << " type=" << static_cast<int>(container->GetContainerType()));
    return true;
}

bool ContainerController::ContainerSupported() {
    return !containers_.empty();
}

// ─────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────

int ContainerController::RunCommand(const std::string& cmd) {
    LOG_DEBUG("ContainerController::RunCommand: " << cmd);
    return system(cmd.c_str());
}

} // namespace adsw_manager
} // namespace ros2
