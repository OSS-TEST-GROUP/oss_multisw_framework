cmake_minimum_required(VERSION 3.5)

project ( "ota_client" )

## Sub Directory


## Source
set ( MAIN_SRC main.cc )

## Library
#link_libraries ( pthread )
#link_directories ( ${CMAKE_INSTALL_PREFIX} )

## Target binary
#add_library ( abc_static STATIC or SHARED or OBJECT $<TARGET_OBJECTS:abcd_obj> )
#set_target_properties ( abc_static PROPERTIES OUTPUT_NAME abc )
#target_link_libraries ( abc_static ${} )

#add_executable ( ${PROJECT_NAME} ${MAIN_SRC} $<TARGET_OBJECTS:abc_obj> )
add_executable ( ${PROJECT_NAME} ${MAIN_SRC} )

## Install
install ( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
