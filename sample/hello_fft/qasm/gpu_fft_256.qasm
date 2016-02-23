##############################################################################
# UNIFORMS

:header
# Uniforms and data offsets per QPU
# Length of a block for each QPU: number of unifoms + 2 * number of passes
# The values for the passes are byte offsets for the memory address for each QPU.
.rep q, 8
    .if q == 0
        # First: 8 byte header with some meta infos
        .byte (:start  - :header)      / 8 # Offset to code start
        .byte (:0_pass - :0_start - 4) / 4 # number of uniforms per QPU, excluding data addresses
        .byte (:0_end  - :0_pass  + 4) / 8 # number of passes; 1 implies that in place operation is allowed
        .byte (:0_tw   - :0_start)     / 4 # add twiddles start to this location
        .byte (:0_dir  - :0_start)     / 4 # patch direction flag into this location
        .byte (:0_src1 - :0_start)     / 4 # put the first sorce address here rather than behind the static uniforms
        .byte (:loop   - :start)       / 8 # Code offset to loop for alignment
        .align 8                           # required by vc4asm
    .endif

:0_start      # once per QPU
:0_tw   .int  4                                               # Twiddles root, skip 1.0
        .byte q, reverseBits(q,3), reverseBits(~q,3), reverseBits(16-q,4)
:0_src1 .int  16 * q                                          # source address
        .short :sync - :loop + (q<<5)                         # ra_sync
        .short :end - :end_base                               # rb_end
        .int  vpm_setup(2, -16, h32(16+q, 0))                 # VPM pass 2
:0_dir  .int  0.                                              # Direction
        .int  vpm_setup(4, 1, v32(16*(q>>2), 4*(q&3)))        # VPM pass 1
        .int  vdw_setup_0(16, 2, dma_h32(16*(q>>2), 4*(q&3))) # VDW pass 2
:0_pass       # per batch data excluding first source
        .int  (q&5)<<8 | (q&2)<<6                             # destination addess
:0_end
.endr


##############################################################################
# REGISTER

.set ra_link_0,         ra0
.set rb_loop,           rb0
.set ra_tmp,            ra1
.set rb_tmp,            rb1
.set ra_link_1,         ra2
.set rb_swap,           rb2
.set ra_vpm_2,          ra3
.set rb_vpm_1,          rb3
.set ra_vdw,            ra4  # ra_vdw and ra_sync share the same register
.set ra_sync,           ra4  # but ra_vdw is only significant for element 0 and ra_sync only for element 15
.set rb_end,            rb4
.set ra_aoff_x,         ra5
.set rb_addr_x,         rb5
.set rb_addr_y,         rb6

.set ra_tw_21,          ra7
.set rb_tw_20,          rb7
.set ra_dir_1,          ra8
.set rb_dir_2,          rb8

# Stage 1 twiddles, constant
.set ra_ti_2,           ra10
.set rb_tr_2,           rb10
.set ra_ti_3,           ra11
.set rb_tr_3,           rb11
# stage 2 twiddles, mutable
.set ra_ti_4,           ra12
.set rb_tr_4,           rb12
.set ra_ti_5,           ra13
.set rb_tr_5,           rb13
.set ra_ti_6,           ra14
.set rb_tr_6,           rb14
.set ra_ti_7,           ra15
.set rb_tr_7,           rb15

.set ra_0x400,          ra27
.set ra_elem,           ra28
.set ra_m1f,            ra29
.set ra_inst,           ra30


##############################################################################
# SETUP
#
# *tmu0 = [t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4, t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4]
# *tmu0 = [0:X0r, X16r, X32r, X48r, X64r ... X112r, 8:X0i   ... X112i] + q
# *tmu0 = [0:X128r, X144r, X160r, X176r  ... X240r, 8:X128i ... X240i] + q
# *tmu0 = [0:X1r, X17r, X33r, X49r, X65r ... X113r, 8:X1i   ... X113i] + q
# *tmu0 = [0:X129r, X145r, X161r, X177r  ... X241r, 8:X129i ... X241i] + q
# ra_dir_1 = [D, D, D, D, D, D, D, D, -D, -D, -D, -D, -D, -D, -D, -D]
# rb_dir_2 = [-D, D, -D, D, -D, D, -D, D, -D, D, -D, D, -D, D, -D, D]
# rb_swap  = [1,0,1,0, -1,1,-1,1, 1,0,1,0, -1,1,-1,1]
# ra_inst  = 8a: logical QPU number, 8b: bit reversed, 8c: -QPU bit reversed, 8d: 0
# rb_vpm_1 = vpm_setup(4, 1, v32(16*(q>>2), 4*(q&3)))
# ra_vpm_2 = vpm_setup(2, -16, h32(16+q, 0))
# ra_vdw[0] = vdw_setup_0(16, 2, dma_h32(16*(q>>2), 4*(q&3)))
# ra_sync[15] = :sync - :loop + (q<<5)
# ra_elem  = e:321032102103210_321032103210____
# rb_tr_2  = [t1, t1,-t0,  -t0,   t1/2 ...  t1/2,-t1/2,-t1/2]
# ra_ti_2  = [t0, t0, t1,   t1,   t1/2 ... -t1/2,-t1/2,-t1/2] * dir
# rb_tr_3  = [t1,-t0, t1/2,-t1/2, t3/4 ...  t1,-t0, t1/2,-t1/2 ... -t1/4, t1/4,-t3/4]
# ra_ti_3  = [t0, t1, t1/2, t1/2, t1/4 ... -t0,-t1,-t1/2,-t1/2 ... -t3/4,-t3/4,-t1/4] * dir
# rb_tw_20 = &tw[00+8q, 00+1q, 00+2q, 40-1q,  00+4q, 20+1q, 40-2q, 20-1q,  40-8q, 10+1q, 20+2q, 30-1q,  40-4q, 30+1q, 20-2q, 10-1q]
# ra_tw_21 = &tw[40-8q, 08+1q, 10+2q, 38-1q,  20+4q, 28+1q, 30-2q, 18-1q,  00+8q, 18+1q, 30+2q, 28-1q,  20-4q, 38+1q, 10-2q, 08-1q]
:start
    shl ra_elem.8abcd, elem_num, 4;  mov r5rep, 4
.if 1
    #ldi      r0,    [0, 0,-0,-0,  1, 1, 1, 1,  0,-0, 1, 1,  2, 3, 3, 2]
    #ldi.setf rb_tmp,[1, 1,-1,-1,  1, 1, 1, 1,  1,-1, 1, 0,  1, 1, 0, 0]
    ldi      r0,    [-0,0, 1, 1,  3, 2, 2, 3,  -0,0, 1, 1,  3, 2, 2, 3]
# rb_tw_20 = &tw[40-8q, 40-1q, 40-2q, 00+1q,  40-4q, 20-1q, 00+2q, 20+1q,  00+8q, 30-1q, 20-2q, 10+1q,  00+4q, 10-1q, 20+2q, 30+1q]
# ra_tw_21 = &tw[00+8q, 38-1q, 30-2q, 08+1q,  20-4q, 18-1q, 10+2q, 28+1q,  40-8q, 28-1q, 10-2q, 18+1q,  20+4q, 08-1q, 30+2q, 38+1q]
.else
    ldi      r0,    [0,-0, 1, 1,  2, 3, 3, 2,  0,-0, 1, 1,  2, 3, 3, 2]
    ldi.setf rb_tmp,[1,-1, 1, 1,  1, 1, 1, 1,  1,-1, 1, 0,  1, 1, 0, 0]
.endif
    mov      ra_m1f, -1.;            mul24    r0, r0, r5
    add      t0s,   unif, r0;        mov      ra_tmp, unif            # <= twiddles,   -> Twiddles for pass 1
    shl      ra_0x400, r5, 8;
    shr      r0,    ra_elem, 5;      mov      r3, ra_elem
    sub      r1,    ra_0x400, r5;    mov      ra_inst, unif           # <= QPU number, r1 = 0x3fc
    and      r0,    r0, r1;          v8adds   ra_elem.8abcd, elem_num, r3 # r0 = e:210____3__
    add      r1,    r0, unif;                                         # <= source address
    mov      t0s,   r1;              add      rb_addr_x, r1, 8        # -> X0
    #ldi.setf r2,    [1, 0, 0, 3,  0, 0, 2, 1,  0, 0, 0, 2,  1, 0, 1, 2] # NZ = negative increment
    ldi.setf r2,    [0, 3, 2, 0,  1, 1, 0, 0,  1, 2, 1, 0,  0, 2, 0, 0] # NZ = negative increment
    add      t0s,   r1, ra_0x400;    mov      ra_aoff_x, r0           # -> X1
    shl.ifnz r2,    2,  r2;          v8adds.ifz r0, ra_inst.8b, ra_inst.8b # r2 = offset for q0, l0
                                     mov.ifnz r0, ra_inst.8d
    ldi.setf rb_tmp,[-1,1, 1, 1,  1, 1, 1, 1,  -1,0, 0, 1,  1, 0, 1, 1] # N = pass1: -4 -> 0, Z = 
    sub.ifnz r2,    0,  r2;          
    ldi      r1,    [1,-2,-1,-2,  0,-2,-1,-2,  1,-2,-1,-2,  0,-2,-1,-2] # rotate right
    ror.setf r0,    r0, r1;          mov      ra_sync, unif           # <= jump offsets
    #ldi      r3,    [0, 0, 0, 3,  0, 1, 1, 2,  0, 2, 1, 1,  0, 3, 0, 0] # base offset
    ldi      r3,    [0, 3, 1, 0,  0, 2, 0, 1,  0, 1, 0, 2,  0, 0, 1, 3] # base offset
    ror      r1,    4,  r1;          v8adds   r0, r0, r3
    shl      r0,    r0, 2;           mov      ra_vpm_2, unif          # <= VPM pass 2
    ldi      rb_swap, [1,0,1,0, -1,1,-1,1, 1,0,1,0, -1,1,-1,1]
    xor      r3,    r0, r1;          mov      rb_end,  ra_sync.16b
    add.ifz  r0,    r0, r2;          mov      ra_sync, ra_sync.16a
.if 0
# *tmu0 = [t0, t0,  t1,  t1, t1/2,t1/2,t1/2,t1/2, t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4]
# rb_tr_2  = [t1, t1,-t0,  -t0,   t1/2 ...  t1/2,-t1/2,-t1/2]
# ra_ti_2  = [t0, t0, t1,   t1,   t1/2 ... -t1/2,-t1/2,-t1/2] * dir
# rb_tr_3  = [t1,-t0, t1/2,-t1/2, t3/4 ...  t1,-t0, t1/2,-t1/2 ... -t1/4, t1/4,-t3/4]
# ra_ti_3  = [t0, t1, t1/2, t1/2, t1/4 ... -t0,-t1,-t1/2,-t1/2 ... -t3/4,-t3/4,-t1/4] * dir
    ldtmu0
    mov r0, r4; mov r1, r4
                                     mov.if3  r1, r0>>8
                                     mov.ifn3 r0, r0<<8
# r0 = [t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4, t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4]
# r1 = [t0, t0,  t1,  t1, t1/2,t1/2,t1/2,t1/2, t0, t0,  t1,  t1, t1/2,t1/2,t1/2,t1/2]
                                     fmul     ra_ti_2, r1, ra_dir_1
                                     fmul     ra_ti_3, r0, ra_dir_1
                                     mov.ifn1 rb_tr_2, r1<<2
                                     fmul.if1 rb_tr_2, r1>>2, ra_m1f
                                     mov.ifn0 rb_tr_3, r0<<1
                                     fmul.if0 rb_tr_3, r0>>1, ra_m1f
                                     
.else
                                     mov      r5rep,   unif           # <= direction
    shl.setf r2,    ra_elem, 28;     mov      ra_dir_1, r5            # C = e:0, N = e:3
    fmul.ifn  ra_dir_1, r5, ra_m1f;  mov.ifc  rb_dir_2, r5            # .if3.if0
    fmul.ifcc rb_dir_2, r5, ra_m1f;  add.setf -,       r2, r2;        # Z = e:210 == 0
    add      rb_tw_20, r0, ra_tmp;   mov.ifz  r3,      r0<<8
    add      ra_tw_21, r3, ra_tmp;   mov      rb_vpm_1, unif;  ldtmu0 # <= VPM pass 1, <- twiddles
    mov.ifnz r2,    r4;              mov.ifz  r2,      0
    shl.setf -, elem_num, 31;        mov      r1,      0              # C = e:1, N = e:0
    mov      r3,    ra_m1f;          mov      r5rep,   r4<<2
    fsub.ifc r1,    r1, r3;          mov.ifnn rb_tr_3, r4<<1          # .if1.if0
    add.setf ra_tmp, ra_elem, ra_elem;  fmul.ifn rb_tr_3, r2>>1, r3>>1   # C = e:3, N = e:2
    mov.ifn  r1,    r5;              fmul     ra_ti_3, r2, ra_dir_1   # .if2
    mov      t0s,   rb_addr_x;       fmul     ra_ti_2, r1, ra_dir_1   # -> X1
    shl.setf -,     elem_num, 30;    mov      rb_tr_2, r1<<2          # C = e:2, N = e:1
    mov      ra_elem.8c, ra_tmp;     fmul.ifn rb_tr_2, r1>>2, r3>>2   # .if1
    add      t0s, rb_addr_x, ra_0x400;                                # -> X129
    mov.ifz  ra_vdw, unif;           mov      vw_setup, rb_vpm_1;  ldtmu0 # <= VDW pass 2
.endif
    # pass 1
    .back 3
    brr ra_link_1, rb_loop, -, r:pass_1
    .endb
:loop
# VPM	0   	1   	2   	...	15
# 0 	x0r 	x0i 	x1r 		x7i
# 1 	x16r	...
# 2 	x32r	...
# ...
# 15	x240r	... 	 		x247i
# 16	x8r 	x8i 	x9r 		x15i
# ...
# 31	x248r	x248i	x249r		x255i
# QPU	0   	0   	1   		7
    brr ra_link_0, r:pass_1                                   # link immediately to :sync
                                      mov t0s, rb_tw_20       # -> Twiddles for pass 2
    mov r0,        rb_loop;           mov t0s, ra_tw_21
    add ra_link_1, r0, ra_sync;       read vw_wait;  ldtmu0

# pass 2

# VPM	0   	1   	2   	...	15	QPU
# 0 	X0r 	X0i 	X1r 		X7i	0
# 1 	X128r	X128i	X129r		X135i	1
# 2 	X64r	X64i	X65r		X71i	2
# ...
# 7 	X224r	X224i	X225r		X231i	7
# 8 	X16r	X16i	X17r		X23i	0
# ...
# 15	X240r	X240i	X241r		X247i	7
# 16	X8r 	X8i 	X9r 		X15i	0
# 17	X136r	X136i	X137r		X143i	1
# ...
# 31	X248r	X248i	X249r		X255i	7

    brr      ra_link_1, r5rep, -, r:pass_2_tw
    add.setf rb_addr_y, unif, unif;       mov vr_setup, ra_vpm_2  # <= destination address/2, msb = last flag
                                          mov r3,       1.
    add.ifc  ra_link_1, r5, rb_end;       mov vw_setup, ra_vpm_2;  ldtmu0  # return to :end on the last turn
:end_base

    v8adds   vr_setup,  ra_vpm_2, 8;      v8adds r0,    ra_vpm_2, 8
    add      t0s,       unif, ra_aoff_x;  mov    r1,    ra_aoff_x # <= next source address
    add      r2,        rb39, r1;         mov    r3,    ra_m1f    # -> X0, X128, next batch
    add      t0s,       r2, ra_0x400;     v8adds rb_addr_x, r2, 8
    add      ra_link_1, rb_loop, ra_sync; mov vw_setup, r0;  ldtmu0
    .back 3
    brr      ra_link_0, r:pass_2_tw                               # link immediately to :sync + 16q
    .endb

# VPM	0   	1   	2   	...	15
# 0 	X0r 	X0i 	X2r 		X14i
# 1 	X128r	X128i	X130r		X142i
# 2 	X64r	X64i	X66r		X78i
# 3 	X192r	X192i	X194r		X206i
# ...
# 7 	X224r	X224i	X226r		X238i
# 8 	X16r	X16i	X18r		X30i
# ...
# 15	X240r	X240i	X242r		X254i
# 16	X1r 	X1i 	X3r 		X15i
# 17	X129r	X129i	X131r		X143i
# ...
# 31	X241r	X241i	X243r		X255i

# VPM	0   	1   	2   	...	15
# 0 	Z0r 	Z0i 	Z64r		Z112i
# 1 	Z1r 	Z1i 	Z65r		Z113i
# 2 	Z2r 	Z2i 	Z66r		Z114i
# 3 	Z3r 	Z3i 	Z67r		Z115i
# ...
# 7 	Z7r 	Z7i 	Z71i		Z119i
# 8 	Z8r 	Z8i 	Z72i		Z120i
# ...
# 15	Z15r	Z15i	Z79r		Z127i
# 16	Z128r	Z128i	Z192r		Z240i
# 17	Z129r	Z129i	Z193r		Z241i
# ...
# 31	Z143r	z143i	z207r		z255i

    add    r1,     ra_vdw, 16;            mov    vw_setup, ra_vdw
    add    r5quad, rb_addr_y, r3;         mov    vw_addr,  rb_addr_y
    mov    t0s,    rb_addr_x;                    
                                          mov    vw_setup, rb_vpm_1 # -> X1, X129, next batch
    add    t0s,    rb_addr_x, r2;         mov    vw_setup, r1
    mov    ra_link_1, rb_loop;            mov    vw_addr,  r5;  ldtmu0
    .back 3
    brr    -,      r:pass_1
    .endb

:end
    brr    ra_link_0, r:pass_2_tw                                 # link immediately to :sync + 16q
    v8adds r0,        ra_vpm_2, 8;        v8adds vr_setup, ra_vpm_2, 8
                                          mov    r3,       ra_m1f
    add    ra_link_1, rb_loop, ra_sync;   mov    vw_setup, r0;  ldtmu0

    mov    r5quad,    rb_addr_y;          mov    vw_setup, ra_vdw
    mov.setf -, ra_inst.8c;               mov    vw_addr,  rb_addr_y
    brr.allz -, r:end_master
                                          mov    vw_setup, rb_vpm_1
                                          add    vw_setup, ra_vdw, 16
    mov    r0, 1;                         add    vw_addr,  r5, r3
#end_slave
    read vw_wait;   thrend
    srel -, 0
    nop

:end_master
    .rep i, 7
    sacq -, 0
    .endr
    read vw_wait;   mov interrupt, r0;  thrend
    nop
    nop
    

##############################################################################
# SUBROUTINES

# needs ldtmu0 before call!!!
:pass_1
    add.setf r3, ra_elem, ra_elem; mov r0, r4;  ldtmu0          # C = bit 3, N = bit 2
.if 0
    mov vpm, r0
    mov vpm, r4
    #mov vpm, rb30
    #mov vpm, ra_ti_2
    nop
.else
# r0 = [0:X0r, X16r, X32r, X48r, X64r ... X112r, 8:X0i   ... X112i]
# r4 = [0:X128r, X144r, X160r, X176r  ... X240r, 8:X128i ... X240i]
    fsub r2, r4, r0
    fadd r0, r0, r4;              fmul.ifn r1, r2>>4, ra_m1f    # .if:2
.if 0
    mov vpm, r0
    mov vpm, r2
.else
# r0 =  [0:x0r, x1r, x2r,  x3r,  4:x4r  ... x7r,  8:x0i  ... x7i]
# r2 = -[0:x8r, x9r, x10r, x11r, 4:x12r ... x15r, 8:x8i  ... x15i]
# fr0  = xr0 + xr4*1  - xi4*0  = r0' = r0     + r0<<4           [0..3]
# fr8  = xr8 + xr12*0 - xi12*D = r0' = -r2>>4 + (r2*rb_dir)<<8  [4..7]
# fi0  = xi0 + xi4*1  + xr4*0  = r0' = r0     + r0<<4           [8..11]
# fi8  = xi8 + xi12*0 + xr12*D = r0' = -r2>>4 + (r2*rb_dir)>>8  [12..15]
# fr4  = xr0 - xr4*1  + xi4*0  = r2' = r0     - r0<<4           [0..3]
# fr12 = xr8 - xr12*0 + xi12*D = r2' = -r2>>4 - (r2*rb_dir)<<8  [4..7]
# fi4  = xi0 - xr4*1  - xi4*0  = r2' = r0     - r0<<4           [8..11]
# fi12 = xi8 - xr12*0 - xi12*D = r2' = -r2>>4 - (r2*rb_dir)>>8  [12..15]
    mov.ifnn r1, r0;              fmul.ifn r2, r2<<8, ra_dir_1  # .if:2
    add.setf ra_tmp, r3, r3;      mov.ifnn r2, r0<<4            # C = bit 2, N = bit 1
    fadd     r0, r1, r2;
# r0    = [0:x0r, x1r, x2r, x3r, 4:x8r  ... x11r, 8:x0i ... x11i]
# r1-r2 = [0:x4r, x5r, x6r, x7r, 4:x12r ... x15r, 8:x4i ... x15i]
.if 0
    mov vpm, r0
    mov vpm, r2
.else
    fsub.ifn r3, r1, r2;          mov.ifnn r3, r0<<2            # .if:1
    add.setf rb_tmp, ra_tmp, ra_tmp; fmul  ra_tmp, r3, rb_tr_2  # C = bit 1, N = bit 0, .if:1
# r0 = [0:x0r, x1r, x4r, x5r, 4:x8r  ... x13r, 8:x0i ... x13i]
# r1 = [0:x2r, x3r, x6r, x7r, 4:x10r ... x15r, 8:x2i ... x15i]
# fr0 = xr0 + xr2*tr - xi2*ti = r0' = r0 + r1*tr - (r1* ti)<<8  [0..7]
# fi0 = xi0 + xi2*tr + xr2*ti = r0' = r0 + r1*tr - (r1*-ti)>>8  [8..15]
# fr2 = xr0 - xr2*tr + xi2*ti = r2' = r0 - r1*tr + (r1* ti)<<8  [0..7]
# fi2 = xi0 - xr2*tr - xi2*ti = r2' = r0 - r1*tr + (r1*-ti)>>8  [8..15]
    fsub.ifcc r2, r1, r2;         fmul     r1, r3<<8, ra_ti_2
    fsub     r1, ra_tmp, r1;      mov.ifc  r0, r2>>2            # .if:1
    fadd     r2, r0, r1;          mov      ra_tmp, rb_tmp
.if 0
    mov vpm, r2
    mov vpm, r0
.else
    fsub.ifn r3, r0, r1;          mov.ifnn r3, r2<<1            # .if:0
    add.setf -, ra_tmp, ra_tmp;   fmul ra_tmp, r3,    rb_tr_3   # C = bit 0, N = bit 3, .if:0
# r2 = [0:x0r, x2r, x4r, x6r, 4:x8r ... x14r, 8:x0i ... x14i]
# r1 = [0:x1r, x3r, x5r, x7r, 4:x9r ... x15r, 8:x1i ... x15i]
    fsub.ifcc r0, r0, r1;         fmul     r1, r3<<8, ra_ti_3
    fsub     r1, ra_tmp, r1;      mov.ifc  r2, r0>>1            # .if:0
    fadd     r0, r2, r1
    fsub     r2, r2, r1;          mov      r3, rb_swap
.if 0
    mov vpm, r0
    mov vpm, r2
.else
    mov.ifn  r1, r2;              mov.ifnn r1, r0<<8            # .if:3
    mov.setf -, r3;               mov.ifn  r0, r2>>8
.if 0
    nop
    mov vpm, r0
    mov vpm, r1
.else
# r0 = [0:x0r, x2r, x4r, x6r, 4:x8r ... x14r, 8:x1r ... x15r]
# r1 = [0:x0i, x2i, x4i, x6i, 4:x8i ... x14i, 8:x1i ... x15i]
# bit rotate for final vdw  [2,3,6,7] <-> [8,9,12,13]
    mov.ifnn r3, r1;              mov.ifn  r3, r1>>3
    mov.ifnn r2, r0;              mov.ifn  r2, r0>>3
                                  mov.ifz  r2, r0<<3
    mov vpm, r2;                  mov.ifz  r3, r1<<3
# r2 = [0:x0r, x8r, x4r, x12r, 4:x2r ... x14r, 8:x1r ... x15r]
# r3 = [0:x0i, x8i, x4i, x12i, 4:x2i ... x14i, 8:x1i ... x15i]
    mov vpm, r3
.endif
.endif
.endif
.endif
.endif
.endif

    .back 3
    bra -, ra_link_1
    .endb

:pass_2_tw
.if 0
    nop
    bra -, ra_link_1
    itof vpm, elem_num
    mov vpm, r4
    nop
.endif
# unpack twiddle factors for pass 2
# input:
#   *tmu0 = [t01, t30, t20, t31, t10, t32, t21, t33, t00, t34, t22, t35, t11, t36, t23, t37]
#   r3    = multiplier for rb_tr_4
# output:
#   rb_tr_4 = [t01, t01, t01, t01, t01, t01 ... t01, t01]  requires *-1 for loop 2
#   ra_ti_4 = [t00,-t00, t00,-t00, t00,-t00 ... t00,-t00]  * dir
#   rb_tr_5 = [t11, t11, t11, t11, t11, t11 ...-t10,-t10]
#   ra_ti_5 = [t10,-t10, t10,-t10, t10,-t10 ... t11,-t11]  * dir
#   rb_tr_6 = [t21, t21, t21, t21,-t20,-t20 ...-t22,-t22]
#   ra_ti_6 = [t20,-t20, t20,-t20, t21,-t21 ... t23,-t23]  * dir
#   rb_tr_7 = [t31, t31,-t30,-t30, t33,-t33,-t32 ...-t36,-t36]
#   ra_ti_7 = [t30,-t30, t31,-t31, t32,-t32, t33 ... t37,-t37]  * dir
    .lset ra_0, ra_ti_4
    mov      r0, r4;               fmul      r5rep, r4, r3          # r5 = rb_tr_4
    mov      r1, vpm;              fmul      ra_tmp, vpm, r5
    shl.setf -, ra_elem, 24;       mov       r5rep, r0<<8           # C = e:0, N = e:3
    mov      ra_0, 0.;             fmul      r3, r5, rb_dir_2       # r3 = ra_ti_4
    mov      r5quad, r4;           fmul.ifc  r2, r1>>1, r3>>1       # .if:0
    add.setf -, ra_elem, ra_elem;  fmul.ifcc r2, r1<<1, r3<<1       # C = e:3, N = e:2
    fsub     r2, ra_tmp, r2;       mov.ifnc  r5quad, r0<<4          # .if:2
    fadd     r0, vpm, r2;          mov       r1, vpm
# r0    = [x0r, x0i, x1r ... 8:x4r  ... x7i]
# r1-r2 = [x8r, x8i, x9r ... 8:x12r ... x15i]
.if 0
    fsub vpm, r1, r2
    mov vpm, r0
.else
    fsub.ifc  r3, r1, r2;          fmul      ra_ti_5, r5, rb_dir_2  # .if:3
    shl.setf  -, ra_elem, 24;      mov.ifcc  r3, r0<<8              # C = e:0, N = e:3, .if:3
    fsub.ifnn r1, r1, r2;          fmul.ifc  r2, r3>>1, ra_ti_5>>1  # .if:30
    mov.ifn   r1, r5;              fmul.ifcc r2, r3<<1, ra_ti_5<<1
    fsub.ifnn r1, ra_0, r5;        mov.ifn   r0, r1>>8              # .if:3
    shl.setf  -, ra_elem.16b, 24;  mov       r1, r1<<8              # C = e:0, N = e:2
                                   fmul      r3, r3, r1
    fsub      r2, r3, r2;          mov       r5quad, r4<<2
    mov       r3, r5;              fmul      ra_ti_6, r5, rb_dir_2
    fadd      r1, r0, r2;          mov.ifn   r5quad, r3>>4
.if 0
    fsub vpm, r0, r2
    mov vpm, r1
    #mov vpm, r0
    #mov vpm, ra_ti_5
.else
# r1    = [x0r, x0i, x1r ... 8:x8r  ... x11i]
# r0-r2 = [x4r, x4i, x5r ... 8:x12r ... x15i]
    fsub.ifn  r3, r0, r2;          mov.ifnn  r5quad, r3<<4
    fsub.ifn  r5quad, ra_0, r5;    mov.ifnn  r3, r1<<4              # .if:2
    fsub.ifnn r0, r0, r2;          fmul.ifc  r2, r3>>1, ra_ti_6>>1  # .if:0
                                   fmul.ifcc r2, r3<<1, ra_ti_6<<1
                                   fmul      r3, r3, r5
    fsub      r2, r3, r2;          mov.ifn   r1, r0>>4              # .if:2
    mov.ifc   r3, r4;              mov.ifcc  r3, r4<<1              # .if:0
    fadd      r0, r1, r2;          fmul      ra_ti_7, r3, rb_dir_2
.if 0
    fsub vpm, r1, r2
    mov vpm, r0
.else
# r0    = [x0r, x0i, x1r ... 8:x8r  ... x13i]
# r1-r2 = [x2r, x2i, x3r ... 8:x10r ... x15i]
    shl.setf  -, ra_elem.8c, 30;   mov       rb_tr_7, r3<<2         # C = e:1, N = e:0
                                   fmul.ifc  rb_tr_7, r3>>2, ra_m1f # .if:1
    fsub.ifc  r3, r1, r2;          mov.ifcc  r3, r0<<2              # .if:1
    fsub.ifcc r1, r1, r2;          fmul.ifn  r2, r3>>1, ra_ti_7>>1  # .if:0
                                   fmul.ifnn r2, r3<<1, ra_ti_7<<1
                                   fmul      r3, r3, rb_tr_7
    fsub      r2, r3, r2;          mov.ifc   r0, r1>>2              # .if:1
    fsub      vpm, r0, r2
    fadd      vpm, r0, r2;
# *vpm : [x2r, x2i, x3r ... 8:x10r ... x15i]
# *vpm : [x0r, x0i, x1r ... 8:x8r  ... x13i]
.endif
.endif
.endif

    .back 3
    bra -, ra_link_1
    .endb

#.align 32, :loop
# output:
#  r2 = 0x400
#  r3 = 0x200
:sync
    .rep i, 7
:       bra  -, ra_link_0
        srel -, 0
        shr  r3, ra_0x400, 1;  mov r2, ra_0x400
        sacq -, i+1
    .endr
:   # entry point for master
    shr  r3, ra_0x400, 1;  mov r2, ra_0x400
    .rep i, 7
        sacq -, 0
    .endr
    .rep i, 7
        srel -, i+1
    .endr
    .back 3
        bra -, ra_link_0
    .endb

