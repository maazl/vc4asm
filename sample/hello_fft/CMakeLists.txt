cmake_minimum_required(VERSION 2.6)

# set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Path to vc4asm, need to be aligned to the HOST system rather than the target system.
set(VC4ASMBINPATH "../../build-${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}")

set (ASM_FILES
	qasm/gpu_fft_256.qasm
	qasm/gpu_fft_512.qasm
	qasm/gpu_fft_1k.qasm
	qasm/gpu_fft_2k.qasm
	qasm/gpu_fft_4k.qasm
	qasm/gpu_fft_8k.qasm
	qasm/gpu_fft_16k.qasm
	qasm/gpu_fft_32k.qasm
	qasm/gpu_fft_64k.qasm
	qasm/gpu_fft_128k.qasm
	qasm/gpu_fft_256k.qasm
	qasm/gpu_fft_512k.qasm
	qasm/gpu_fft_1024k.qasm
	qasm/gpu_fft_2048k.qasm
	qasm/gpu_fft_4096k.qasm
	qasm/gpu_fft_trans.qasm
)

add_library(fftCommonObjects OBJECT
	gpu_fft.c
	gpu_fft_base.c
	gpu_fft_shaders.c
	gpu_fft_twiddles.c
	mailbox.c
)

set(1D_SOURCE_FILES
	hello_fft.c
)

set(2D_SOURCE_FILES
	hello_fft_2d.c
	gpu_fft_trans.c
)

unset(HEX_FILES)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hex)

unset(DIS_FILES)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dis)

foreach(ASM_FILE ${ASM_FILES})
	get_filename_component(FILENAME ${ASM_FILE} NAME_WE)
	string(SUBSTRING ${FILENAME} 8 -1 SHADER_SIZE)
	set (HEX_FILE shader_${SHADER_SIZE}.hex)
	add_custom_command(OUTPUT ${HEX_FILE}
		COMMAND ${VC4ASMBINPATH}/vc4asm -V -v -C hex/${HEX_FILE}
			-i vc4.qinc
			${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE}
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE} ${VC4ASMBINPATH}/vc4asm)
	set (DIS_FILE shader_${SHADER_SIZE}.dis)
	add_custom_command(OUTPUT ${DIS_FILE}
		COMMAND ${VC4ASMBINPATH}/vc4dis -v -x -o dis/${DIS_FILE} hex/${HEX_FILE}
		DEPENDS ${HEX_FILE} ${VC4ASMBINPATH}/vc4dis)
	list(APPEND DIS_FILES ${DIS_FILE})
endforeach()

set_source_files_properties(${HEX_FILES} PROPERTIES GENERATED TRUE)
add_custom_target( hello_fft_assembled_hexs DEPENDS ${HEX_FILES} )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties(${DIS_FILES} PROPERTIES GENERATED TRUE)
add_custom_target( hello_fft_disassembled_hexs DEPENDS ${DIS_FILES} )

add_executable(hello_fft ${1D_SOURCE_FILES} $<TARGET_OBJECTS:fftCommonObjects>)
add_dependencies(hello_fft hello_fft_assembled_hexs hello_fft_disassembled_hexs)
target_link_libraries(hello_fft ${CMAKE_DL_LIBS} m)

add_executable(hello_fft_2d ${2D_SOURCE_FILES} $<TARGET_OBJECTS:fftCommonObjects>)
add_dependencies(hello_fft_2d hello_fft_assembled_hexs hello_fft_disassembled_hexs)
target_link_libraries(hello_fft_2d ${CMAKE_DL_LIBS} m)

