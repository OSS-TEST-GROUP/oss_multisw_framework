cmake_minimum_required(VERSION 3.5)

project ( "dds_idl_generate" )
message("Start dds_idl_generate")

# Set path to fastrtpsgen or use RTI generator
set(FASTDDSGEN "fastddsgen")  # Adjust this path
set(IDL_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Output directory for generated files
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")

if (NOT EXISTS ${GENERATED_DIR})
    file(MAKE_DIRECTORY ${GENERATED_DIR})

    # Function to generate sources from IDL files
    function(generate_dds_from_idl IDL_DIR IDL_FILE)
        get_filename_component(IDL_FILE_DIR ${IDL_FILE} DIRECTORY)
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        file(MAKE_DIRECTORY ${GENERATED_DIR}/${IDL_FILE_DIR})

        execute_process(
            COMMAND ${FASTDDSGEN} -I ${IDL_DIR} -replace -d ${GENERATED_DIR}/${IDL_FILE_DIR} ${IDL_DIR}/${IDL_FILE}
        )

        # Optionally expose generated files for further build steps
        set(GENERATED_SOURCES ${GENERATED_DIR}/${IDL_FILE_DIR}/${IDL_NAME}PubSubTypes.cxx PARENT_SCOPE)
    endfunction()

    # Find all .idl files in service directories
    file(GLOB_RECURSE IDL_FILES RELATIVE ${IDL_FILE_DIR} "*.idl")

    foreach(IDL_FILE ${IDL_FILES})
        generate_dds_from_idl(${IDL_FILE_DIR} ${IDL_FILE})
    endforeach()
endif()
