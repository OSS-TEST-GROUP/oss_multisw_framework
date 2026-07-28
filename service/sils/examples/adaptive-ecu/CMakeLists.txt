# adaptive-ecu -- Case 2 example: an AUTOSAR Adaptive SWC on libsils_vecu.
#
# Header-only developer code (SWC + ara::com shim + adapter) + main.cc; links the same
# targets as steer-ecu. No AP stack is required to BUILD this example -- the shim stands in
# for ara::com. A production build would instead link the vendor AP runtime and drop the
# shim (src/ara_com_shim.hpp).
add_executable(adaptive_ecu src/main.cc)
target_include_directories(adaptive_ecu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(adaptive_ecu PRIVATE sils_vecu sils_vehicle_idl)

install(TARGETS adaptive_ecu RUNTIME DESTINATION bin)
