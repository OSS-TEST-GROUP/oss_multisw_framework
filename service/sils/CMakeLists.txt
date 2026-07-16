cmake_minimum_required(VERSION 3.5)

project ( "sils" )

## Sub Directory
add_subdirectory ( vecu )

# Example developer ECUs built on the libsils_vecu SDK (docs/sils-extension-model.md P2).
# Added after vecu so the sils_vecu target exists. ON by default to dogfood the SDK
# boundary in every SiLS build; -DSILS_BUILD_EXAMPLES=OFF to skip.
option(SILS_BUILD_EXAMPLES "Build the example developer ECUs (brake-ecu, ...)" ON)
if(SILS_BUILD_EXAMPLES)
    add_subdirectory ( examples )
endif()
