    mov vpm, r0;       mov r1, vpm       #O VPM read and write can coexsist
    mov vpm, r0;       mov vw_setup, r1  #W Must mot write VPM setup concurrently to VPM write
    mov r0, vpm;       mov vw_setup, r1  #W ... nor VPM read
    mov vpm, r0;       mov vr_setup, r1  #W Must mot write VPM setup concurrently to VPM write
    mov r0, vpm;       mov vr_setup, r1  #W ... nor VPM read
    mov vr_setup, r0;  mov vw_setup, r1  #W concurrent VPM read/write setup
