add_library(commonObjects OBJECT
    utils.cpp
    Message.cpp
    expr.cpp
    Inst.cpp
    Eval.cpp
    Validator.cpp
)

set(ASM_SOURCE_FILES
    AssembleInst.cpp
    Parser.cpp
    vc4asm.cpp
)

IF(CMAKE_SYSTEM_NAME STREQUAL Linux)
   list(APPEND ASM_SOURCE_FILES WriteELF.cpp)   
ENDIF()

set(DIS_SOURCE_FILES
    Disassembler.cpp
    vc4dis.cpp
)

add_executable(vc4asm ${ASM_SOURCE_FILES} $<TARGET_OBJECTS:commonObjects>)
add_executable(vc4dis ${DIS_SOURCE_FILES} $<TARGET_OBJECTS:commonObjects>)

install(TARGETS vc4asm vc4dis RUNTIME DESTINATION bin)
