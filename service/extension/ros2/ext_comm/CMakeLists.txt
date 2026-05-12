cmake_minimum_required(VERSION 3.5)

project ( "ext_comm" )

# Aggregator for extension-side communication components. Today only
# dds_bridge (rosbridge ↔ bare-DDS translator) lives here; future
# inter-extension daemons (status forwarders, log relays, etc.) belong
# under this subdir.
add_subdirectory ( dds_bridge )
