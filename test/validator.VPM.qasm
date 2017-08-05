    mov vpm, r0;       mov r1, vpm       #O VPM read and write can coexsist
    mov vpm, r0;       mov vw_setup, r1  #W:V110.3 Must mot write VPM setup concurrently to VPM write
    mov r0, vpm;       mov vw_setup, r1  #W:V110.2 ... nor VPM read
    mov vpm, r0;       mov vr_setup, r1  #W:V110.3 Must mot write VPM setup concurrently to VPM write
    mov r0, vpm;       mov vr_setup, r1  #W:V110.2 ... nor VPM read
    mov vr_setup, r0;  mov vw_setup, r1  #W:V110.1 concurrent VPM read/write setup
