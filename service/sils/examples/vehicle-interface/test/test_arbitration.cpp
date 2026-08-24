// test_arbitration.cpp -- VCU 아비트레이션 규칙의 불변식 검사.
//
// 왜 여기서 하나: 규칙이 틀렸을 때 증상이 "차가 좀 이상하다" 로만 나타난다. 버스에 올려
// 놓고 보면 어느 ECU 때문인지, 규칙 때문인지, 플랜트 때문인지 못 가른다. 규칙은 순수
// 함수이므로 ROS·DDS·시뮬레이터 없이 여기서 못박는다 -- adapters/ss2/run_tests.sh 와 같은 형태.
//
// 검사하는 것은 docs/sils/sils-vecu-arbitration.md §4.1 의 불변식이다. 전부 **반증 가능한**
// 형태로만 쓴다 -- "VCU 가 옳다" 는 증명하지 않는다. 명백히 틀린 것을 걸러낼 뿐이다.

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "vehicle_interface_behavior.hpp"

namespace {

int checks = 0;
int failures = 0;

void Check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::printf("  FAIL: %s\n", what.c_str());
    }
}

void CheckNear(double got, double want, const std::string& what) {
    Check(std::fabs(got - want) < 1e-9,
          what + " (got " + std::to_string(got) + ", want " + std::to_string(want) + ")");
}

sils::ControlRequest Req(const std::string& name, double accel, double steer = 0.0) {
    sils::ControlRequest r;
    r.name = name;
    r.sample.acceleration = accel;
    r.sample.steering_angle = steer;
    r.sample.gear = 3;
    return r;
}

sils::VehicleBehaviorContext Ctx(bool have_target, double target_accel, double target_steer,
                                 std::vector<sils::ControlRequest> reqs) {
    sils::VehicleBehaviorContext c;
    c.have_target = have_target;
    c.target.acceleration = target_accel;
    c.target.steering_angle = target_steer;
    c.target.gear = 3;
    c.requests = std::move(reqs);
    return c;
}

// I1 -- 출력 accel <= 모든 입력 accel 의 최소.
// 이 프레임워크가 스펙 없이 주장할 수 있는 거의 유일한 명제다: 어떤 vECU 도 차를 더
// 위험하게 만들 수 없다.
void TestI1_MonotoneSafety() {
    VehicleInterfaceBehavior b(2.0);

    // 피어가 더 강하게 제동 -> 피어가 이긴다
    auto c1 = b.OnTick(Ctx(true, 1.5, 0.0, {Req("Request.adaptive", -3.0)}));
    CheckNear(c1.acceleration, -3.0, "I1: 더 강한 제동이 이긴다");
    Check(b.last_decision().long_winner == "Request.adaptive", "I1: 승자가 기록된다");

    // 피어가 더 세게 가속하려 함 -> 무시된다 (이 규칙의 핵심 제약)
    auto c2 = b.OnTick(Ctx(true, 1.5, 0.0, {Req("Request.plainc", 8.0)}));
    CheckNear(c2.acceleration, 1.5, "I1: 피어는 가속을 못 늘린다");
    Check(b.last_decision().long_winner == "owner", "I1: owner 가 유지된다");

    // 여럿 -> 최소
    auto c3 = b.OnTick(Ctx(true, 1.0, 0.0,
                           {Req("a", 8.0), Req("b", -0.5), Req("c", -4.25), Req("d", 2.0)}));
    CheckNear(c3.acceleration, -4.25, "I1: 여럿이면 최소가 이긴다");
    Check(b.last_decision().long_winner == "c", "I1: 최소를 낸 채널이 승자");
}

// 순서 무관 -- min 의 교환·결합법칙이 실제로 성립하는지. 이것이 깨지면 S3 lockstep
// 골든 재현이 깨진다. 같은 집합을 순서만 바꿔 넣어 결과가 같은지 본다.
void TestOrderIndependence() {
    VehicleInterfaceBehavior b(2.0);
    const std::vector<sils::ControlRequest> set = {Req("a", -1.0), Req("b", -3.5), Req("c", 0.5)};

    auto forward = b.OnTick(Ctx(true, 1.0, 0.0, set));
    const std::string w1 = b.last_decision().long_winner;

    std::vector<sils::ControlRequest> reversed(set.rbegin(), set.rend());
    auto backward = b.OnTick(Ctx(true, 1.0, 0.0, reversed));
    const std::string w2 = b.last_decision().long_winner;

    CheckNear(backward.acceleration, forward.acceleration, "순서 무관: 명령이 같다");
    Check(w1 == w2, "순서 무관: 승자도 같다");

    std::vector<sils::ControlRequest> rotated = {set[1], set[2], set[0]};
    auto rot = b.OnTick(Ctx(true, 1.0, 0.0, rotated));
    CheckNear(rot.acceleration, forward.acceleration, "순서 무관: 회전시켜도 같다");
}

// I2 -- 무입력 -> 감속. 통신 두절이 "0 을 명령" 이 되면 안 된다.
void TestI2_NoTargetIsDeceleration() {
    VehicleInterfaceBehavior b(2.0);
    auto c = b.OnTick(Ctx(false, 0.0, 0.0, {}));
    Check(c.acceleration < 0.0, "I2: target 없으면 감속");
    CheckNear(c.acceleration, -2.0, "I2: safe_decel 크기만큼");
    CheckNear(c.steering_angle, 0.0, "I2: 조향 중립");
    Check(c.gear == 3, "I2: 기어 DRIVE 유지");
    Check(b.last_decision().long_winner == "safe-stop", "I2: safe-stop 이 기록된다");

    // safe stop 도 규칙의 극단값일 뿐이다 -- 피어는 그보다 더 세게 제동할 수 있다.
    auto c2 = b.OnTick(Ctx(false, 0.0, 0.0, {Req("Request.classic", -5.0)}));
    CheckNear(c2.acceleration, -5.0, "I2: 피어는 safe stop 보다 더 제동할 수 있다");
    // 반대로 약하게는 못 만든다.
    auto c3 = b.OnTick(Ctx(false, 0.0, 0.0, {Req("Request.classic", -0.1)}));
    CheckNear(c3.acceleration, -2.0, "I2: 피어가 safe stop 을 약화시키지 못한다");
}

// 횡축 -- clamp 만. 피어가 새 조향값을 주입할 수 없다.
void TestLateralClampOnly() {
    VehicleInterfaceBehavior on(2.0, 0.5);
    auto c1 = on.OnTick(Ctx(true, 0.0, 1.2, {}));
    CheckNear(c1.steering_angle, 0.5, "횡축: 상한으로 clamp");
    Check(on.last_decision().steer_clamped, "횡축: clamp 사실이 기록된다");

    auto c2 = on.OnTick(Ctx(true, 0.0, -1.2, {}));
    CheckNear(c2.steering_angle, -0.5, "횡축: 하한으로 clamp");

    auto c3 = on.OnTick(Ctx(true, 0.0, 0.1, {}));
    CheckNear(c3.steering_angle, 0.1, "횡축: 범위 안이면 그대로");
    Check(!on.last_decision().steer_clamped, "횡축: clamp 안 했으면 기록도 없다");

    // 피어가 큰 조향을 내도 출력에 안 들어간다 -- 주입 금지
    auto c4 = on.OnTick(Ctx(true, 0.0, 0.1, {Req("a", 0.0, 0.49)}));
    CheckNear(c4.steering_angle, 0.1, "횡축: 피어가 조향을 주입하지 못한다");

    // 기본값 0 = OFF (차량 한계를 모르므로 지어내지 않는다)
    VehicleInterfaceBehavior off(2.0);
    auto c5 = off.OnTick(Ctx(true, 0.0, 1.2, {}));
    CheckNear(c5.steering_angle, 1.2, "횡축: 한계 미설정이면 clamp 하지 않는다");
}

// 기어는 owner 만. 피어가 주행 중 기어를 바꾸면 안 된다.
void TestGearOwnerOnly() {
    VehicleInterfaceBehavior b(2.0);
    sils::ControlRequest r = Req("a", -3.0);
    r.sample.gear = 1;  // REVERSE
    auto c = b.OnTick(Ctx(true, 1.0, 0.0, {r}));
    Check(c.gear == 3, "기어: 피어가 바꾸지 못한다 (owner 의 DRIVE 유지)");
    CheckNear(c.acceleration, -3.0, "기어: 종축은 그대로 아비트레이션된다");
}

// NaN -- 깨진 피어가 명령을 가져가면 안 된다. 순진한 비교는 NaN 을 통과시킨다.
void TestNanPeerIgnored() {
    VehicleInterfaceBehavior b(2.0);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    auto c = b.OnTick(Ctx(true, 1.0, 0.0, {Req("broken", nan), Req("ok", -2.5)}));
    Check(!std::isnan(c.acceleration), "NaN: 출력이 NaN 이 아니다");
    CheckNear(c.acceleration, -2.5, "NaN: 성한 피어가 이긴다");
}

// 피어가 없으면 예전 그대로 -- 단일 ECU 구성이 이 변경으로 달라지면 안 된다.
void TestNoPeersIsPassThrough() {
    VehicleInterfaceBehavior b(2.0);
    auto c = b.OnTick(Ctx(true, 0.836088, 0.00055238, {}));
    CheckNear(c.acceleration, 0.836088, "피어 없음: target 을 그대로 통과");
    CheckNear(c.steering_angle, 0.00055238, "피어 없음: 조향도 그대로");
    Check(!b.last_decision().arbitrated, "피어 없음: 아비트레이션 안 했다고 기록");
}

}  // namespace

int main() {
    TestI1_MonotoneSafety();
    TestOrderIndependence();
    TestI2_NoTargetIsDeceleration();
    TestLateralClampOnly();
    TestGearOwnerOnly();
    TestNanPeerIgnored();
    TestNoPeersIsPassThrough();

    std::printf("%s  test_arbitration: %d checks, %d failures\n",
                failures == 0 ? "PASS" : "FAIL", checks, failures);
    return failures == 0 ? 0 : 1;
}
