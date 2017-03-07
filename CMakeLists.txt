cmake_minimum_required(VERSION 2.8)
project(vc4asm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(src)

if(BUILD_SAMPLES)
	add_subdirectory(sample)
endif()

install(FILES share/vc4.qinc DESTINATION share)
