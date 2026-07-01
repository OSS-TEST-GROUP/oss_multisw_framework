// autoware_e2e_common.h — shared helpers for all 9 autoware E2E test suites.
//
// Purpose:
//   - Replace hardcoded AUTOWARE_IMAGE and absolute path constants in each file.
//   - Provide fast-skip prereq check (<1s) so machines without docker / the mock
//     image / MMF binaries skip immediately instead of paying a 30s timeout.
//   - All paths are env-overridable so the unit CI runner can inject them; when
//     the env vars are absent a CMake-injected compile definition provides a
//     correct default for local development too.
//
// Usage in each test file:
//   #include "autoware_e2e_common.h"
//   ... in SetUpTestSuite():
//     std::string skip_reason;
//     if (!AutowareE2EPrereqs(skip_reason)) { GTEST_SKIP() << skip_reason; return; }
//   ... replace AUTOWARE_IMAGE with AutowareImage()
//   ... replace path constants with MmfOutputDir() / MmfLfBin() etc.

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

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
