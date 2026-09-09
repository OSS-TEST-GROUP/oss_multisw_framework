cmake_minimum_required(VERSION 3.5)

project("domain_agent")

include_directories(.)
link_libraries(pthread fastcdr fastdds)

add_executable(${PROJECT_NAME}
    main.cc
    domain_agent.cc
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 런타임에 std_msgs/String.idl 을 동적 IDL 파서로 읽는다. dds_bridge 가 같은
# 디렉토리를 이미 install 하지만, 어느 타깃이 먼저 복사했는지에 의존하지 않도록
# 여기서도 명시한다(dds_bridge 의 lifecycle install 과 같은 이유).
install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/idl/std_msgs
    DESTINATION bin/idl
)
