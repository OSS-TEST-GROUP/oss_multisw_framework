cmake_minimum_required(VERSION 3.5)

project ( "ext_comm" )

# Aggregator for extension-side communication components:
#   dds_bridge   — rosbridge ↔ bare-DDS translator (두 도메인 평면을 잇는다)
#   domain_agent — 원격 HMI Settings 가 정한 도메인을 이 호스트의 domain.env 에 반영
# 향후 inter-extension 데몬(상태 포워더, 로그 릴레이 등)도 이 아래에 둔다.
add_subdirectory ( dds_bridge )
add_subdirectory ( domain_agent )
