cmake_minimum_required(VERSION 3.5)

project ( "service" )

## Sub Directory
if (NOT BUILD_EXTENSION)
	add_subdirectory ( core )
else ()
	add_subdirectory ( extension )
endif()
