set (ASM_FILES
    smitest.qasm
)

set(SOURCE_FILES
    mailbox.c
    smitest.c
)

unset(HEX_FILES)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/hex)
unset(DIS_FILES)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dis)

foreach(ASM_FILE ${ASM_FILES})
    get_filename_component(FILENAME ${ASM_FILE} NAME_WE)
    set (HEX_FILE ${FILENAME}.hex)
    add_custom_command(OUTPUT ${HEX_FILE} COMMAND ${PROJECT_BINARY_DIR}/src/vc4asm -V -C hex/${HEX_FILE}
                    ${PROJECT_SOURCE_DIR}/share/vc4.qinc
                    ${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE})
    list(APPEND HEX_FILES ${HEX_FILE})
    set (DIS_FILE ${FILENAME}.dis)
    add_custom_command(OUTPUT ${DIS_FILE} COMMAND ${PROJECT_BINARY_DIR}/src/vc4dis -x -v2 -M -o dis/${DIS_FILE} hex/${HEX_FILE} DEPENDS ${HEX_FILE})
    list(APPEND DIS_FILES ${DIS_FILE})
endforeach()

set_source_files_properties(${HEX_FILES} PROPERTIES GENERATED TRUE)
add_custom_target(smitest_assembled_hexs DEPENDS ${HEX_FILES} )
add_dependencies(smitest_assembled_hexs vc4asm)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/hex)

set_source_files_properties(${DIS_FILES} PROPERTIES GENERATED TRUE)
add_custom_target(smitest_disassembled_hexs DEPENDS ${DIS_FILES} )
add_dependencies(smitest_disassembled_hexs vc4dis)

add_executable(smitest ${SOURCE_FILES})
add_dependencies(smitest smitest_assembled_hexs smitest_disassembled_hexs)

install(TARGETS smitest RUNTIME DESTINATION bin)
