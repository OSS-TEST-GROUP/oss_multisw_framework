#!/usr/bin/env bash
#
# Unit-test the adapters/ss2 mappings -- both directions of loop B.
#
#   ss2_ego_mapping.hpp      SS_v2 state  -> canonical EgoState        (B2-1, outbound)
#   ss2_control_mapping.hpp  VehicleControl -> Autoware command        (B2-3, return)
#   aw_state_mapping.hpp     orientation, both directions               (stage 3)
#   aw_stamp_policy.hpp      sim-time stamp -> consumer epoch           (stage 3)
#
# Needs ONLY a C++20 compiler -- no ROS 2, no DDS, no simulator, no container. That is
# the point: the arithmetic that can be silently wrong (frames, units, quaternion
# convention, ego selection, the acceleration sign in reverse) is isolated in those two
# headers precisely so it can be checked on any machine, including the ones without a
# Linux simulator host.
#
# Usage:  adapters/ss2/run_tests.sh
# Exit:   0 all checks passed · 1 a check failed or a build failed

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp="${TMPDIR:-/tmp}"
trap 'rm -f "${tmp}"/ss2_*_test.$$' EXIT

CXX="${CXX:-c++}"
status=0

# Each suite builds and runs independently: a compile error in one must not hide the
# other's results, and "the ego mapping still passes" is useful information while the
# control mapping is being written.
for suite in mapping control_mapping state_mapping stamp_policy; do
    out="${tmp}/ss2_${suite}_test.$$"
    echo "==> building test_${suite} (${CXX})"
    if ! "${CXX}" -std=c++20 -Wall -Wextra -Werror \
            -I"${here}/include" \
            "${here}/test/test_${suite}.cpp" \
            -o "${out}"; then
        echo "    BUILD FAILED: test_${suite}"
        status=1
        continue
    fi
    echo "==> running test_${suite}"
    "${out}" || status=1
done

exit "${status}"
