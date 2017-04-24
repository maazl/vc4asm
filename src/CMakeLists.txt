# does not work:  set_property(GLOBAL PROPERTY CXX_STANDARD 11)
# so let's use plan B:
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Work around for the insufficiency of cmake to handle common dependencies of targets w/o building them repeatedly.
# cmake will build each source file as often as it is referenced. To get reasonable build times on Raspberry Pi no file must be referenced more than once.
add_library(objvc4base OBJECT
	utils.cpp
	Message.cpp
	expr.cpp
	Inst.cpp
	Eval.cpp
	Validator.cpp
)
add_library(objvc4asm OBJECT
	AssembleInst.cpp
	Parser.cpp
	WriterBase.cpp
	WriteC.cpp
	WriteELF.cpp
)
add_library(objvc4dis OBJECT
	Disassembler.cpp
	WriteQasm.cpp
)

add_executable(vc4asm vc4asm.cpp $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm>)
add_executable(vc4dis vc4dis.cpp $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4dis>)

# Build static *and* shared library:
# https://cmake.org/pipermail/cmake/2007-January/012669.html
add_library(libvc4asm-static STATIC $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm> $<TARGET_OBJECTS:objvc4dis>)
set_target_properties(libvc4asm-static PROPERTIES OUTPUT_NAME vc4asm CLEAN_DIRECT_OUTPUT 1)
add_library(libvc4asm-shared SHARED $<TARGET_OBJECTS:objvc4base> $<TARGET_OBJECTS:objvc4asm> $<TARGET_OBJECTS:objvc4dis>)
set_target_properties(libvc4asm-shared PROPERTIES OUTPUT_NAME vc4asm CLEAN_DIRECT_OUTPUT 1)
