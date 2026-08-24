#ifndef ROS2_ADSW_MANAGER_SERIVCE_OS_TYPE_HPP_
#define ROS2_ADSW_MANAGER_SERIVCE_OS_TYPE_HPP_

namespace ros2 {
namespace adsw_manager {
    
    // idl/dds/lifecycle/LifecycleCommon.idl 의 MwType 미러. 순서가 그쪽과 같아야 한다.
    // MW_NATIVE  = 미들웨어 프레임워크 없이 도는 순수 프로세스/컨테이너 (service/extension/general)
    // MW_UNKNOWN = 변환 실패 sentinel. 없으면 fallback 이 ROS2 로 떨어져 거짓 라우팅이 된다.
    enum class MwType {
        ROS2,
        AGL,
        AUTOSAR,
        MW_NATIVE,
        MW_UNKNOWN
    };
    
} // namespace adsw_manager
} // namespace ros2

#endif // ROS2_ADSW_MANAGER_SERIVCE_OS_TYPE_HPP_