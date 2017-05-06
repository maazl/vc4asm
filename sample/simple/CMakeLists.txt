cmake_minimum_required(VERSION 2.6)

# set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Path to vc4asm, need to be aligned to the HOST system rather than the target system.
set(VC4ASMBINPATH "../../build-${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}")

set(SOURCE_FILES
	mailbox.c
	smitest.c
)

add_custom_command(OUTPUT shader.o shader.h COMMAND ${VC4ASMBINPATH}/vc4asm -V
	-e shader.o -h shader.h ../../share/vc4inc/vc4.qinc smitest.qasm
	DEPENDS ../../share/vc4inc/vc4.qinc smitest.qasm ${VC4ASMBINPATH}/vc4asm
	MAIN_DEPENDENCY smitest.qasm)

add_executable(smitest ${SOURCE_FILES} shader.o)

