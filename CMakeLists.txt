cmake_minimum_required(VERSION 2.6)

# set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Force out of source build. Otherwise multiple platform builds will not work.
# http://stackoverflow.com/questions/11143062/getting-cmake-to-build-out-of-source-without-wrapping-scripts
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	project(stub NONE)
	set(OUT_OF_SOURCE_DIR "build-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}")

	message(WARNING "In-source builds not allowed. CMake will now be run with arguments: cmake -H. -B${OUT_OF_SOURCE_DIR}")

	# Run CMake with out of source flag
	execute_process(
		COMMAND ${CMAKE_COMMAND} -H. -B${OUT_OF_SOURCE_DIR}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

	if (MAKE_IMMEDIATELY)
		execute_process(
			COMMAND make
			WORKING_DIRECTORY ${OUT_OF_SOURCE_DIR})
	endif()

	# Cause fatal error to stop the script from further execution
	# It would be more pretty if we could return a zero result here but everything else seems not to prevent a meaningless Makefile to be generated.
	message(FATAL_ERROR "CMake has been ran to create an out of source build => abort. This is NOT an error!")
endif ()


project(vc4asm)

# Place results in the top level
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(src)

# install TARGETS links the executables a second time, taking twice a much time just for nothing.
# install(TARGETS vc4asm vc4dis RUNTIME DESTINATION bin)
# work around: use install PROGRAMS
install(PROGRAMS $<TARGET_FILE:vc4asm> $<TARGET_FILE:vc4dis> DESTINATION bin)
install(PROGRAMS $<TARGET_FILE:libvc4asm-static> $<TARGET_FILE:libvc4asm-shared> DESTINATION lib)
install(DIRECTORY share/vc4inc share/vc4tmpl DESTINATION share)

