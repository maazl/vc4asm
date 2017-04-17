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

# does not work:  set_property(GLOBAL PROPERTY CXX_STANDARD 11)
# so let's use plan B:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Work around for the insufficiency of cmake to handle common dependencies of targets w/o building them repeatedly.
# cmake will build each source file as often as it is referenced. To get reasonable build times on Raspberry Pi no file must be referenced more than once.
add_library(objvc4base OBJECT
	src/utils.cpp
	src/Message.cpp
	src/expr.cpp
	src/Inst.cpp
	src/Eval.cpp
	src/Validator.cpp
)
add_library(objvc4asm OBJECT
	src/AssembleInst.cpp
	src/Parser.cpp
	src/WriterBase.cpp
	src/WriteC.cpp
	src/WriteELF.cpp
)
add_library(objvc4dis OBJECT
	src/Disassembler.cpp
)

add_executable(vc4asm src/vc4asm.cpp $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm>)
add_executable(vc4dis src/vc4dis.cpp $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4dis>)

# Build static *and* shared library:
# https://cmake.org/pipermail/cmake/2007-January/012669.html
add_library(libvc4asm-static STATIC $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm> $<TARGET_OBJECTS:objvc4dis>)
set_target_properties(libvc4asm-static PROPERTIES OUTPUT_NAME libvc4asm CLEAN_DIRECT_OUTPUT 1)
add_library(libvc4asm-shared SHARED $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm> $<TARGET_OBJECTS:objvc4dis>)
set_target_properties(libvc4asm-shared PROPERTIES OUTPUT_NAME libvc4asm CLEAN_DIRECT_OUTPUT 1)

# install TARGETS links the executables a second time, taking twice a much time just for nothing.
# install(TARGETS vc4asm vc4dis RUNTIME DESTINATION bin)
# work around: use install PROGRAMS
install(PROGRAMS $<TARGET_FILE:vc4asm> $<TARGET_FILE:vc4dis> DESTINATION bin)
install(PROGRAMS $<TARGET_FILE:libvc4asm-static> $<TARGET_FILE:libvc4asm-shared> DESTINATION lib)
install(DIRECTORY share/vc4inc share/vc4tmpl DESTINATION share)

