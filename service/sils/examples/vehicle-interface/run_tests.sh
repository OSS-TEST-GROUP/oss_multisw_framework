#!/usr/bin/env bash
#
# VCU 아비트레이션 규칙 단위 테스트.
#
# C++20 컴파일러만 있으면 된다 -- ROS 없음, DDS 없음, 시뮬레이터 없음, 컨테이너 없음.
# 그게 요점이다: 조용히 틀릴 수 있는 것(순서 의존, NaN, 안전 방향 뒤집힘)을 버스에
# 올리기 전에 못박는다. adapters/ss2/run_tests.sh 와 같은 형태.
#
# Usage: service/sils/examples/vehicle-interface/run_tests.sh
# Exit:  0 통과 · 1 실패 또는 빌드 실패

set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "${here}/../../../.." && pwd)"
tmp="${TMPDIR:-/tmp}"
out="${tmp}/vi_arbitration_test.$$"
trap 'rm -f "${out}"' EXIT

CXX="${CXX:-c++}"
echo "==> building test_arbitration (${CXX})"
"${CXX}" -std=c++20 -Wall -Wextra -Werror \
    -I"${here}/src" -I"${root}/service/sils/vecu/src" \
    "${here}/test/test_arbitration.cpp" -o "${out}"
echo "==> running test_arbitration"
"${out}"
