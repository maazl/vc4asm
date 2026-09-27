.macro labeled_repeat
:0 .rep i, 2
    mov r0, i
.endr
.endm

labeled_repeat

.if 0
:1 .rep i, -1
    mov r0, 99
.endr
.endif

.rep x, 2
    .rep y, 2
        mov r0, x*4+y
    .endr
.endr

.rep x, 1
    .if x == 0
        mov r0, 11
    .else
        mov r0, 12
    .endif
.endr

.foreach z, 7, 9
    mov r0, z
.endfor
