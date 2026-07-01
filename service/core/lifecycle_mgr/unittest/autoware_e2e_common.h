// autoware_e2e_common.h — shared helpers for all 9 autoware E2E test suites.
//
// Purpose:
//   - Replace hardcoded AUTOWARE_IMAGE and absolute path constants in each file.
//   - Provide fast-skip prereq check (<1s) so machines without docker / the mock
//     image / MMF binaries skip immediately instead of paying a 30s timeout.
//   - All paths are env-overridable so the unit CI runner can inject them; when
//     the env vars are absent a CMake-injected compile definition provides a
//     correct default for local development too.
//   - Provide per-run DDS domain isolation so tests running concurrently with a
//     live oss-mmf-core/oss-mmf-ext-ros2 stack (domain 0) do not cross-talk.
//
// Usage in each test file:
//   #include "autoware_e2e_common.h"
//   ... in SetUpTestSuite():
//     std::string skip_reason;
//     if (!AutowareE2EPrereqs(skip_reason)) { GTEST_SKIP() << skip_reason; return; }
//     int domain = E2EDomainId();
//     std::string profile = E2EWriteFastDdsProfile(domain);
//   ... replace AUTOWARE_IMAGE with AutowareImage()
//   ... replace path constants with MmfOutputDir() / MmfLfBin() etc.
//   ... pass domain / profile to StartContainer(), docker exec, and child setenv.

#pragma once

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// ── Image selector ──────────────────────────────────────────────────────────
//
// Priority: env MMF_AUTOWARE_TEST_IMAGE > compile default "mmf-autoware-mock:humble"
// Set MMF_AUTOWARE_TEST_IMAGE=ghcr.io/autowarefoundation/autoware:... to test
// against the real image without editing source.
inline std::string AutowareImage() {
    const char* env = std::getenv("MMF_AUTOWARE_TEST_IMAGE");
    if (env && strlen(env) > 0) return env;
    return "mmf-autoware-mock:humble";
}

// ── MMF root resolver ───────────────────────────────────────────────────────
//
// Priority: env MMF_E2E_ROOT > CMake compile definition MMF_E2E_ROOT_DEFAULT
// MMF_E2E_ROOT_DEFAULT is injected by CMakeLists.txt as CMAKE_SOURCE_DIR so
// local builds work without any env setup.
inline std::string MmfRoot() {
    const char* env = std::getenv("MMF_E2E_ROOT");
    if (env && strlen(env) > 0) return env;
#ifdef MMF_E2E_ROOT_DEFAULT
    return MMF_E2E_ROOT_DEFAULT;
#else
    // Last-resort fallback: current repo checkout path on the original dev machine.
    // CI must set MMF_E2E_ROOT to avoid this.
    return "/home/obigo/project/oss/oss_mmf_agent/src/oss_multisw_framework";
#endif
}

// ── Output directory ────────────────────────────────────────────────────────
//
// Priority: env MMF_E2E_OUTPUT > MmfRoot()+"/output/x86_64/Debug"
inline std::string MmfOutputDir() {
    const char* env = std::getenv("MMF_E2E_OUTPUT");
    if (env && strlen(env) > 0) return env;
    return MmfRoot() + "/output/x86_64/Debug";
}

// ── Derived binary / library / config paths ─────────────────────────────────
inline std::string MmfLfBin()   { return MmfOutputDir() + "/core/bin/lifecycle_mgr"; }
inline std::string MmfAdswBin() { return MmfOutputDir() + "/extension/ros2/bin/adsw_manager"; }
inline std::string MmfLfLib()   { return MmfOutputDir() + "/core/lib"; }
inline std::string MmfAdswLib() { return MmfOutputDir() + "/extension/ros2/lib"; }
inline std::string MmfLfConfig() { return MmfRoot() + "/service/core/lifecycle_mgr/config"; }
inline std::string MmfFastDdsProfile() {
    return MmfRoot() + "/docker/build_environment/fastdds-profile.xml";
}

// FastDDS SDK lib path — isolated env var because it lives outside the repo.
// Priority: env MMF_E2E_FASTDDS_LIB > $HOME/obigo-sdk/Fast-DDS/output/ros2_x86_64/lib
inline std::string MmfFastDdsLib() {
    const char* env = std::getenv("MMF_E2E_FASTDDS_LIB");
    if (env && strlen(env) > 0) return env;
    const char* home = std::getenv("HOME");
    std::string h = (home && strlen(home) > 0) ? home : "/home/obigo";
    return h + "/obigo-sdk/Fast-DDS/output/ros2_x86_64/lib";
}

// Convenience: build LD_LIBRARY_PATH for child processes.
// Prepends LF lib + ADSW lib + FastDDS lib, then appends existing LD_LIBRARY_PATH.
inline std::string MmfBuildLdPath() {
    std::string ld = MmfLfLib() + ":" + MmfAdswLib() + ":" + MmfFastDdsLib();
    const char* existing = std::getenv("LD_LIBRARY_PATH");
    if (existing && strlen(existing) > 0) ld += std::string(":") + existing;
    return ld;
}

// ── Per-run DDS domain isolation ────────────────────────────────────────────
//
// E2EDomainId() returns a unique-per-process DDS domain in [1,200].
// Domain 0 is reserved for the production oss-mmf-core/oss-mmf-ext-ros2 stack.
// Using PID % 200 + 1 guarantees a fresh domain for each test binary invocation.
//
// E2EWriteFastDdsProfile(domain) writes a per-run Fast DDS XML profile to
// /tmp/mmf_e2e_fastdds_<domain>.xml.  It reads the repo's canonical profile
// (MmfFastDdsProfile()) and injects <domainId>N</domainId> into the default
// participant block so every create_participant_with_default_profile() call
// joins domain N instead of domain 0.  The file is per-process-unique so
// parallel test binaries never share or overwrite each other's profile.
//
// Container isolation: pass "-e ROS_DOMAIN_ID=N" when starting the container
// with docker run so that ros2 node list (CycloneDDS inside the container)
// also uses domain N.  docker exec inherits the container's environment, so
// subsequent ros2 node list / ros2 run calls inside the container are
// automatically on domain N without further flags.
inline int E2EDomainId() {
    // Stable within one process; different for each test binary invocation.
    static int domain = 1 + (static_cast<int>(getpid()) % 200);
    return domain;
}

// Read the canonical Fast DDS profile and inject <domainId>N</domainId>,
// write the result to a per-domain temp file, and return that path.
// Falls back to the unmodified canonical profile path if any I/O step fails
// (preserving old behaviour rather than crashing the test suite).
inline std::string E2EWriteFastDdsProfile(int domain) {
    std::string src_path = MmfFastDdsProfile();
    std::string dst_path = "/tmp/mmf_e2e_fastdds_" + std::to_string(domain) + ".xml";

    // Read canonical profile
    std::ifstream src(src_path);
    if (!src.is_open()) return src_path;  // fallback: unmodified (domain 0)
    std::string content((std::istreambuf_iterator<char>(src)),
                         std::istreambuf_iterator<char>());
    src.close();

    // Inject <domainId>N</domainId> immediately after the <rtps> opening tag
    // inside the is_default_profile="true" participant block.
    // The canonical profile has a single participant block with <rtps>; we
    // insert the domain element right after the first <rtps> occurrence.
    // If the profile already has <domainId> we replace it to stay idempotent.
    const std::string domain_tag = "<domainId>" + std::to_string(domain) + "</domainId>";

    // Remove any existing <domainId>...</domainId>
    std::string::size_type d_start = content.find("<domainId>");
    while (d_start != std::string::npos) {
        std::string::size_type d_end = content.find("</domainId>", d_start);
        if (d_end == std::string::npos) break;
        d_end += std::string("</domainId>").size();
        content.erase(d_start, d_end - d_start);
        d_start = content.find("<domainId>");
    }

    // Fast DDS XML schema: <domainId> must be a direct child of <participant>,
    // NOT inside <rtps>. Insert right before the opening <rtps> tag.
    // The canonical profile structure is:
    //   <participant profile_name="..." is_default_profile="true">
    //       <rtps> ... </rtps>
    //   </participant>
    // We want:
    //   <participant profile_name="..." is_default_profile="true">
    //       <domainId>N</domainId>
    //       <rtps> ... </rtps>
    //   </participant>
    std::string::size_type rtps_pos = content.find("<rtps>");
    if (rtps_pos != std::string::npos) {
        content.insert(rtps_pos, domain_tag + "\n        ");
    }

    // Write result
    std::ofstream dst(dst_path);
    if (!dst.is_open()) return src_path;  // fallback
    dst << content;
    dst.close();
    return dst_path;
}

// ── Prerequisite checker ────────────────────────────────────────────────────
//
// Returns true if all prerequisites are satisfied.  Returns false and sets
// skip_reason to a human-readable message if any prereq is absent.
//
// Checked conditions (in order):
//   1. docker daemon is reachable (`docker info` succeeds)
//   2. The mock image (AutowareImage()) is present (`docker image inspect` succeeds)
//   3. lifecycle_mgr binary exists and is non-zero
//   4. adsw_manager binary exists and is non-zero
//
// This function is intentionally fast (<1s on a machine without docker) because
// each condition short-circuits on failure without any sleep or timeout.
inline bool AutowareE2EPrereqs(std::string& skip_reason) {
    // 1. Docker daemon accessible
    if (system("docker info >/dev/null 2>&1") != 0) {
        skip_reason = "docker daemon not reachable (docker info failed) — "
                      "set up docker access or install docker to run autoware E2E tests";
        return false;
    }

    // 2. Mock image present
    std::string inspect_cmd =
        "docker image inspect " + AutowareImage() + " >/dev/null 2>&1";
    if (system(inspect_cmd.c_str()) != 0) {
        skip_reason = "autoware mock image '" + AutowareImage() + "' not found — "
                      "run: docker build -f docker/test/Dockerfile.autoware-mock "
                      "-t mmf-autoware-mock:humble .  "
                      "or set MMF_AUTOWARE_TEST_IMAGE to point to an existing image";
        return false;
    }

    // 3. lifecycle_mgr binary present and non-zero
    struct stat st{};
    std::string lf_bin = MmfLfBin();
    if (stat(lf_bin.c_str(), &st) != 0 || st.st_size == 0) {
        skip_reason = "lifecycle_mgr binary missing or zero-size: " + lf_bin + " — "
                      "run ./build.sh --build-unittest to rebuild; "
                      "or set MMF_E2E_ROOT / MMF_E2E_OUTPUT to point to your build";
        return false;
    }

    // 4. adsw_manager binary present and non-zero
    std::string adsw_bin = MmfAdswBin();
    if (stat(adsw_bin.c_str(), &st) != 0 || st.st_size == 0) {
        skip_reason = "adsw_manager binary missing or zero-size: " + adsw_bin + " — "
                      "run ./build.sh --build-unittest to rebuild";
        return false;
    }

    return true;
}
