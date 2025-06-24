cmake_minimum_required(VERSION 3.5)

project ( "dds_idl_generate" )
message("Start dds_idl_generate")

# Set path to fastrtpsgen or use RTI generator
set(FASTDDSGEN "fastddsgen")  # Adjust this path
set(IDL_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Output directory for generated files
set(GENERATED_DIR "${CMAKE_INSTALL_PREFIX}/generated/idl")
set(BUILD_CHECK_PREFIX "${GENERATED_DIR}/.build_check" )

if (NOT EXISTS ${GENERATED_DIR})
    file(MAKE_DIRECTORY ${GENERATED_DIR})
endif()


function(get_file_timestamp FILE_PATH OUTPUT_VAR)
    if(EXISTS "${FILE_PATH}")
        file(TIMESTAMP "${FILE_PATH}" TIMESTAMP_STR)
        set(${OUTPUT_VAR} "${TIMESTAMP_STR}" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

# Function to generate sources from IDL files
function(generate_dds_from_idl IDL_DIR IDL_FILE)
    get_filename_component(IDL_FILE_DIR ${IDL_FILE} DIRECTORY)
    get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
    file(MAKE_DIRECTORY ${GENERATED_DIR}/${IDL_FILE_DIR})

    get_file_timestamp(${IDL_DIR}/${IDL_FILE} FILE_TIMESTAMP)

    string(REPLACE "/" "_" BUILD_CHECK_IDL_PATH ${IDL_FILE} )
    set(BUILD_CHECK ${BUILD_CHECK_PREFIX}_${BUILD_CHECK_IDL_PATH})

    if (EXISTS ${BUILD_CHECK})
        file(READ ${BUILD_CHECK} LAST_TIMESTAMP)
        if (${LAST_TIMESTAMP} STREQUAL ${FILE_TIMESTAMP})
            return()
        endif()
    endif()

    execute_process(
        COMMAND ${FASTDDSGEN} -I ${IDL_DIR} -replace -d ${GENERATED_DIR}/${IDL_FILE_DIR} ${IDL_DIR}/${IDL_FILE}
        RESULT_VARIABLE GEN_RESULT
    )

    if (NOT GEN_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate DDS for ${IDL_FILE}")
    else ()
        file (WRITE ${BUILD_CHECK} ${FILE_TIMESTAMP} )
    endif ()
endfunction()

# Find all .idl files in service directories
file(GLOB_RECURSE IDL_FILES RELATIVE ${IDL_FILE_DIR} "*.idl")

foreach(IDL_FILE ${IDL_FILES})
    generate_dds_from_idl(${IDL_FILE_DIR} ${IDL_FILE})
endforeach()
