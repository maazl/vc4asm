# Parser tests
    fmul     r0, r0<<8, r1      #W <<8 silently applies to r1
    fmul     r2, r2<<8, r1<<8   #O
    fmul     r0, r0<<8, ra0     #O OK, ra0 will ignore <<8
    fmul     r2, r2<<7, ra0     #W ... but not <<7
    fmul     r0, r0<<7, ra0<<3  #O Uh, well, ra0 will ignore the higher bits of <<
    fmul     r2>>8, r2, ra0     #O
    fmul     r2>>8, r4, ra0     #W neither r4 nor ra0 cares about <<8
    fmul     r2>>1, r4, ra0     #O but the MUL ALU rotation within a slice still work
    mov      r0, r5<<3          #W r5 makes no sense as rotation source at all

# Validator tests
    mov      r1, 0
    mov      r0, r1<<1  #W back to back acces to r1
    mov.ifz  r1, r0<<1  #O well, ifz might not assign the critical higher elements
    mov.ifnz r0, r1>>1  #O the other way around
    mov.ifz  r1, r0<<4  #O
    mov.ifnz r0, r1>>4  #O
    mov.ifnz r1, r0>>1  #O
    mov.ifc  r0, r1<<1  #O
    ldtmu0
    mov      r0, r4<<1  #O r4 does not support rotation over slice boundaries anyway

