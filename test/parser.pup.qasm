# pack tests
    mov.pack8a ra0, r0                  #O
    mov ra1.8a, r0                      #O
    mov r0, rb0.8a                      #E:A43.1 rb cannot unpack
    add r0, ra0.8dr, ra0                #W:A46.2 implicit unpack at 2nd operand
    add r0, ra0, ra0.8dr                #W:A46.1 implicit unpack at 1st operand
    mov r0, ra0; mov r1, ra0.8a         #E:A45.2 implicit unpack other ALU
    mov r0, ra0.8a; mov r1, ra0         #E:A45.1 ... and the other way around
    mov r0, ra0.16b; mov r1, ra0.8a     #E:A70 Conflicting unpack mode
    mov r0, ra0.8a; mov r1, ra0.8a      #O
    mov.pack16a r0, r1                  #E:A63.10 r0 cannot pack
    mov.pack16a ra0.16a, r1             #E:A60 duplicate pack
    add ra1.8c, ra1.8c, ra1.8c; mov.ifnn rb0, r2<<2 #O
    mov ra2.16b, 0; fmul.ifn rb0, r2>>2, (ra2>>2).16b #O

# unpack tests
    mov.unpack8a r0, ra0; mov r1, ra0   #E:A45.1 implicit unpack by other ALU
    mov.unpack8a r0, r4; mov r1, ra0    #O no implicit unpack by other ALU
    mov.unpack8a r0, r4; mov.unpack8a r1, ra0 #E:A70 ... but conflicting PM
    mov.unpack16b r0, r4       		    #O
    mov.unpack16bi r0, r4               #W:A48 r4 can't unpack int16 and there is no regfile A access
    add.unpack16bi r0, r4, ra1          #O but regfile A can
    add.unpack16bf r0, r4, ra1          #O and r4 can unpack float, but r.A can't in case of integer instruction
    add.unpack16bf r0, ra2, r4          #O ... and the other way around
    fadd.unpack16bf r0, r4, ra1         #E:A47.1 but in case of float it is indeterminate
    fadd.unpack16bf r0, ra1, r4         #E:A47.1 ... and the other way around
    mov r0, ra0.16ai; fmul r1, ra0.16af, ra0.16af  #E:A44.1 conflicting float/int mode
    mov r0, ra0.16ai; fmul.unpack16af r1, ra0, ra0 #E:A44.1 ... and with opcode level unpack
    mov r0, ra0.16a; fmul r1, ra0.16a, ra0.16a     #E:A44.1 conflicting float/int mode
    mov r0, ra0.16a; fmul.unpack16a r1, ra0, ra0   #E:A44.1 ... and with opcode level unpack
    mov r0, ra0.16af; fmul r1, ra0.16af, ra0.16af  #O OK, both use float
    v8adds.ifn r3, r3>>8, ra0.8b; mov rb0, -1 #O

# combined
    mov ra1.16af, ra1.16bi              #E:A65.21 mov cannot be an integer and float instruction at once

# packed symbols
.set ra0_8a, ra0.8a
.set ra1_8a, ra0.8a + 1
    mov r0, ra1_8a                      #O
    add r0, ra0_8a, ra0                 #W:A46.2 implicit unpack at 2nd operand
    add r0, ra0, ra0_8a                 #W:A46.1 implicit unpack at 1st operand


