cmake_minimum_required(VERSION 3.5)

project ( "service" )

## Sub Directory
if (NOT BUILD_EXTENSION)
	add_subdirectory ( core )
else ()
	add_subdirectory ( extension )
endif()

# SiLS vECU (Phase 1 vertical-slice PoC). Independent of the core/extension
# switch above -- needs only the project-wide idl/dds generation and FastDDS,
# both already available regardless of BUILD_EXTENSION.
if (BUILD_SILS)
	add_subdirectory ( sils )
endif()
