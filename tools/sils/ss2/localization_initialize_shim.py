#!/usr/bin/env python3
"""localization_initialize_shim -- /localization/initialize 서비스를 토픽 경로로 잇는 다리.

★ 이건 임시방편이지 제품이 아니다. 벤더 요청(docs/sils/sils-autoware-partition-vendor-requests.md)
   은 그대로 유효하다. 이 파일은 그 요청이 반영되기 전까지 우리 rig 를 굴리기 위한 것이고,
   벤더가 자기 shim(planning_sim_localization_state.py)에 서비스 서버를 넣으면 필요 없어진다.

왜 필요한가
-----------
이 파티션 이미지에서 초기화 경로가 둘로 갈린다:

    벤더 sps / RViz  ──▶ /initialpose3d (토픽)  ──▶ planning_sim_localization_state
                                                     └▶ /localization/initialization_state
    SS_v2 concealer  ──▶ /api/localization/initialize (서비스)
                            └▶ ADAPI ──▶ /localization/initialize  ← 서버가 없다

앞의 것만 갖춰져 있다. 원래 뒤쪽 서버는 autoware_pose_initializer 인데 이미지에 설치돼
있지 않고(설치될 이유도 없다 -- 이 파티션은 launch_localization=false 다), 그래서 concealer 가

    AutowareError: Requested the service "/api/localization/initialize" 30 times,
                   but was not successful.

로 죽는다. 서비스는 응답한다 -- ADAPI 가 success=false 를 돌려줄 뿐이다.

이 노드가 하는 일
-----------------
1. /localization/initialize 서버를 세운다 (tier4_localization_msgs/srv/InitializeLocalization).
2. 요청에 담긴 포즈를 /initialpose3d 로 그대로 흘린다 -- 벤더 shim 이 그걸 보고
   initialization_state 를 INITIALIZED 로 바꾼다. 즉 우리가 상태를 지어내지 않고
   **벤더 자신의 메커니즘을 태운다.**
3. success=true 로 응답한다.

포즈를 버리지 않고 흘리는 이유: 이 구성에서 자차 위치는 SS_v2 가 정한다(concealer 가
/vehicle/status/* 와 /localization/* 를 발행). 그래도 값을 넘겨두면 sps 가 켜져 있는
구성이나 앞으로 pose_initializer 가 들어오는 구성에서 그대로 맞는 동작이 된다.
버리면 그때 조용히 틀린 초기위치가 된다.

무엇을 하지 않나
----------------
- 측위를 하지 않는다. method=AUTO 든 DIRECT 든 구별 없이 받아들인다. 이 파티션에는
  정렬할 대상(NDT/GNSS)이 없으므로 AUTO 를 흉내 내는 것은 거짓말이 된다.
- initialization_state 를 직접 발행하지 않는다. 그건 벤더 shim 의 몫이고, 둘이 같은
  토픽에 쓰면 발행자가 둘이 된다 -- 이 저장소가 반복해서 밟은 함정이다.

실행 (제어 파티션 컨테이너 안. rclpy 와 tier4_localization_msgs 가 거기 있다):
    tools/sils/ss2/run_localization_shim.sh --domain-id 42
"""

import rclpy
from geometry_msgs.msg import PoseWithCovarianceStamped
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from tier4_localization_msgs.srv import InitializeLocalization


class LocalizationInitializeShim(Node):
    def __init__(self):
        super().__init__("sils_localization_initialize_shim")
        # 벤더 shim 의 구독(depth 1, RELIABLE, VOLATILE)에 맞춘다. VOLATILE 이라
        # 늦게 뜬 구독자는 과거 샘플을 못 받는다 -- 그래서 아래에서 한 번이 아니라
        # 짧게 여러 번 낸다.
        self._pub = self.create_publisher(
            PoseWithCovarianceStamped,
            "/initialpose3d",
            QoSProfile(
                depth=1,
                reliability=ReliabilityPolicy.RELIABLE,
                durability=DurabilityPolicy.VOLATILE,
            ),
        )
        self._srv = self.create_service(
            InitializeLocalization, "/localization/initialize", self._on_initialize
        )
        self._calls = 0
        self.get_logger().info(
            "serving /localization/initialize -> republish /initialpose3d. "
            "STOPGAP: the vendor image has no pose_initializer; see "
            "docs/sils/sils-autoware-partition-vendor-requests.md"
        )

    def _on_initialize(self, request, response):
        self._calls += 1
        pose = None
        if request.pose_with_covariance:
            pose = request.pose_with_covariance[0]
        else:
            # concealer 는 포즈를 담아 보내지만, 빈 요청도 규격상 유효하다(배열 <=1).
            # 그 경우 지어낼 값이 없으므로 발행을 건너뛴다 -- 0,0,0 을 내면 sps 가
            # 켜진 구성에서 차를 원점으로 순간이동시킨다.
            self.get_logger().warn(
                "initialize request carried NO pose; not publishing /initialpose3d. "
                "state will stay UNINITIALIZED unless something else publishes it."
            )

        if pose is not None:
            msg = PoseWithCovarianceStamped()
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.header.frame_id = pose.header.frame_id or "map"
            msg.pose = pose.pose
            # 벤더 shim 의 구독이 VOLATILE 이고 우리는 요청이 올 때만 발행하므로,
            # 그 순간 shim 이 아직 매칭 전이면 한 번은 놓친다. 몇 번 반복해서 그 창을 덮는다.
            # 내용이 같은 샘플이라 중복 발행의 부작용은 없다(shim 은 래치일 뿐이다).
            for _ in range(3):
                self._pub.publish(msg)
            p = pose.pose.pose.position
            self.get_logger().info(
                f"initialize #{self._calls}: method={request.method} "
                f"pose=({p.x:.3f}, {p.y:.3f}, {p.z:.3f}) -> /initialpose3d"
            )

        response.status.success = True
        response.status.code = 0
        response.status.message = "sils shim: forwarded to /initialpose3d"
        return response


def main():
    rclpy.init()
    node = LocalizationInitializeShim()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
