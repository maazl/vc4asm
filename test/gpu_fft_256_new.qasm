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
        .byte (:0_src1 - :0_start)     / 4 # put the first source address here rather than behind the static uniforms
        .byte (:loop   - :start)       / 8 # Code offset to loop for alignment
        .align 8                           # required by vc4asm
    .endif

:0_start      # once per QPU
:0_tw   .int  4                                               # Twiddles root, skip 1.0
        .byte q, reverseBits(q,3), reverseBits(~q,3), reverseBits(16-q,4)
:0_src1 .int  16 * q                                          # source address
        .short :sync - :loop + (q<<5)                         # ra_sync
        .short :end - :end_base                               # rb_end
        .int  vpm_setup(2, -16, h32(24+q, 0))                 # VPM pass 2
        .int  vpm_setup(4, 1, v32(16*(q>>2), 4*(q&3)))        # VPM pass 1
:0_dir  .int  0.                                              # Direction
        .int  vdw_setup_0(16, 2, dma_h32(16*(q>>2), 4*(q&3))) # VDW pass 2
:0_pass       # per batch data excluding first source
        .int  (q&5)<<8 | (q&2)<<6                             # destination address
:0_end
.endr


##############################################################################
# REGISTER

.set ra_link,           ra0
.set rb_loop,           rb0
.set ra_tmp,            ra1
.set rb_tmp,            rb1
.set ra_inst,           ra2
.set rb_end,            rb2
.set ra_vpm_2,          ra3
.set rb_vpm_1,          rb3
.set ra_vdw,            ra4  # ra_vdw and ra_sync share the same register
.set ra_sync,           ra4  # but ra_vdw is only significant for element 0 and ra_sync only for element 15
.set ra_addr,           ra5
.set rb_addr,           rb5
.set ra_m1f_x400,       ra6
.set rb_m1,             rb6
.set ra_elem,           ra7
.set rb_aoff_x,         rb7

.set ra_dir_1,          ra9
.set rb_dir_2,          rb9

# Pass 1 twiddles, constant
.set ra_ti_2,           ra10
.set rb_tr_2,           rb10
.set ra_ti_3,           ra11
.set rb_tr_3,           rb11
# Pass 2 twiddle pointers, constant
.set rb_tw_20,          rb12
.set rb_tw_21,          rb13


##############################################################################
# SETUP
#
# *tmu0 = [t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4, t0, t1,t1/2,t1/2, t1/4,t3/4,t3/4,t1/4]
# *tmu0 = [0:X0r,  X40r, X20r, X60r, X10r ... X70r, 8:X0i  ... X70i] + 2q
# *tmu1 = [0:X80r, Xc0r, Xa0r, Xe0r, X90r ... Xf0r, 8:X80i ... Xf0i] + 2q
# *tmu0 = [0:X1r,  X41r, X21r, X61r, X11r ... X71r, 8:X1i  ... X71i] + 2q
# *tmu1 = [0:X81r, Xc1r, Xa1r, Xe1r, X91r ... Xf1r, 8:X81i ... Xf1i] + 2q
# ra_dir_1 = [D, D, D, D, D, D, D, D, -D, -D, -D, -D, -D, -D, -D, -D]
# rb_dir_2 = [-D, D, -D, D, -D, D, -D, D, -D, D, -D, D, -D, D, -D, D]
# ra_inst  = 8a: logical QPU number, 8b: bit reversed, 8c: -QPU bit reversed, 8d: 0
# rb_vpm_1 = vpm_setup(4, 1, v32(16*(q>>2), 4*(q&3)))
# ra_vpm_2 = vpm_setup(2, -16, h32(16+q, 0))
# ra_vdw[0] = vdw_setup_0(16, 2, dma_h32(16*(q>>2), 4*(q&3)))
# ra_sync[15] = :sync - :loop + (q<<5)
# ra_elem  = e:321032102103210_3210321032103210
# rb_tr_2  = [t1, t1/2,-t0,  -t1/2, t1, t1/2,-t0,  -t1/2, 8: t1, t1/2,-t0   ... -t1/2]
# ra_ti_2  = [t0, t1/2, t1,   t1/2  t0, t1/2, t1,   t1/2, 8:-t0,-t1/2,-t1   ... -t1/2] * dir
# rb_tr_3  = [t1, t3/4, t1/2, t1/4,-t0,-t1/4,-t1/2,-t3/4, 8: t1, t3/4, t1/2 ... -t3/4]
# ra_ti_3  = [t0, t1/4, t1/2, t3/4, t1, t3/4, t1/2, t1/4, 8:-t0,-t1/4,-t1/2 ... -t1/4] * dir
# rb_tw_20 = &tw[40-8q, 40-1q, 40-2q, 00+1q,  40-4q, 20-1q, 00+2q, 20+1q,  00+8q, 30-1q, 20-2q, 10+1q,  00+4q, 10-1q, 20+2q, 30+1q]
# rb_tw_21 = &tw[00+8q, 38-1q, 30-2q, 08+1q,  20-4q, 18-1q, 10+2q, 28+1q,  40-8q, 28-1q, 10-2q, 18+1q,  20+4q, 08-1q, 30+2q, 38+1q]
:start
    ldi.setf tmu_noswap, [1, 0, 1, 0,  0, 1, 0, 1, -1,-1,-1,-1,-1,-1,-1,-1]
    ldi      r0,    [-0, 1, 0, 1,-0, 1, 0, 1, -0, 3, 1, 2, 0, 2, 1, 3]
    shl      r3,    elem_num, 4;     mul24    r0,       r0, 4
    add      t0s,   unif, r0;        mov      ra_addr,  unif            # <= twiddles, -> Twiddles for pass 1
    mov      r3,    elem_num;        v8adds   ra_elem.8abcd, elem_num, r3
    mov      ra_m1f_x400, 0xbc000400                                    # -1.f16, 0x400
    xor.ifz  r3,    r3, 5;           mov      ra_inst,  unif            # <= QPU number, r1 = 0x3fc
    shl      r3,    r3, 7;           mov      r5rep,    ra_m1f_x400.16a
    ldi      ra_tmp,[0, 3, 1, 0,  0, 2, 0, 1,  0, 1, 0, 2,  0, 0, 1, 3] # base offset
    v8adds.ifn r3, r3>>8, ra_m1f_x400.8b; mov rb_m1,    -1              # r3 = e:012____3__
    add      r1,    r3, unif;        mov      rb_aoff_x, r3             # <= source address
    mov      t0s,   r1;              v8adds   rb_addr,  r1, 8           # -> X0
    ldi.setf r2,    [0, 3, 2, 0,  1, 1, 0, 0,  1, 2, 1, 0,  0, 2, 0, 0] # NZ = negative increment
    add      t1s,   r1, r5;          mov.ifnz r0,       ra_inst.8d      # -> X128
    shl.ifnz r2,    2,  r2;          v8adds.ifz r0, ra_inst.8b, ra_inst.8b # r2 = offset for q0, l0
    ldi.setf rb_tmp,[-1, 1, 1, 1, -1, 1, 1, 1,  -1,0, 0, 1,  1, 0, 1, 1] # N = pass1: 0., Z = negative offset
    sub.ifnz r2,    0,  r2;          mov      ra_sync,  unif            # <= jump offsets
    ldi      r1,    [1,-2,-1,-2,  0,-2,-1,-2,  1,-2,-1,-2,  0,-2,-1,-2] # rotate right
    ror.setf r0,    r0, r1;          mov      rb_end,   ra_sync.16b     # Z = q0 && positive increment
    ror      r1,    4,  r1;          v8adds   r0,       r0, ra_tmp
    shl      r0,    r0, 2;           mov      ra_vpm_2, unif            # <= VPM pass 2
    xor      r3,    r0, r1;          mov      r1,       0
    add.ifz  r0,    r0, r2;          mov      rb_vpm_1, ra32            # <= VPM pass 1, .ifq0 && positive increment
    mov.setf -,     rb_tmp;          mov      vw_setup, ra39;  ldtmu0   # <- TW1
    shl.setf ra_tmp, ra_elem, 28;    mov.ifnn r1,       r4              # C = e:0, N = e:3
    mov.ifnn r2,    r1;              mov.ifn  r2,       r1>>8
    mov.ifc  rb_dir_2, rb32;         fmul.ifn ra_dir_1, rb32, ra_m1f_x400.16b # <= direction, .if3.if0
    mov.ifnn ra_dir_1, rb39;         fmul.ifcc rb_dir_2, rb39, ra_m1f_x400.16b
    add.setf ra_tmp, ra_tmp, ra_tmp; mov.ifnn r1,       r1<<8           # C = e:3, N = e:2, Z = e08, .if3
# r2 = [t0, t1/2, t1,   t1/2  t0, t1/2, t1,   t1/2, 8:-t0,-t1/2,-t1   ... -t1/2]
# r1 = [t0, t1/4, t1/2, t3/4, t1, t3/4, t1/2, t1/4, 8:-t0,-t1/4,-t1/2 ... -t1/4]
    add      rb_tw_20, r0, ra_addr;  mov.ifz  r3,       r0<<8           # .ife08
    mov      t0s,   rb_addr;         fmul     ra_ti_2,  r2, ra_dir_1    # -> X1
    add      t1s,   rb_addr, r5;     fmul     ra_ti_3,  r1, ra_dir_1    # -> X129
    add.setf -,     ra_tmp, ra_tmp;  mov      rb_tr_3,  r1<<4           # C = e:2, N = e:1
    add ra_elem.8c, ra_elem.8c, ra_elem.8c; mov.ifnn rb_tr_2, r2<<2     # .if1
    mov      ra_sync.16b, 0;         fmul.ifn rb_tr_2,  r2>>2, (ra_m1f_x400>>2).16b
                                     fmul.ifc rb_tr_3,  r1>>4, ra_m1f_x400.16b# .if2
    add      rb_tw_21, r3, ra_addr;  mov.ifz  ra_vdw,   unif;  ldtmu0   # <= VDW pass 2, <- X0
# pass 1
    .back 3
    brr      ra_link, r:pass_1
    .endb

    brr      ra_link, r:pass_1
    mov.ifcc r3,    r1;              mov      t0s, rb_tw_20       # -> Twiddles for pass 2, .if:3
    mov      vpm,   r3;              mov.ifcc r0, r1<<8
# r3 = [0:x0r, x8r, x4r, x12r, x2r, x10r, x6r, x14r, 8:x1r ... x15r]
# r0 = [0:x0i, x8i, x4i, x12i, x2i, x10i, x6i, x14i, 8:x1i ... x15i]
    mov      vpm,   r0;              ldtmu0

    brr      ra_link, rb_loop, r:loop, ra_sync
    mov.ifcc r3,    r1;                                           # .if:3
    mov      vpm,   r3;              mov.ifcc r0, r1<<8
    mov      vpm,   r0;              mov      t0s, rb_tw_21
:loop
# VPM	0   	1   	2   	...	15
# 0 	X0r 	X0i 	X1r 		X7i
# 1 	X128r	X128i	X129r		X135i
# 2 	X64r	X64i	X65r		X71i
# ...
# 7 	X224r	X224i	X225r		X231i
# 8 	X16r	X16i	X17r		X23i
# ...
# 15	X240r	X240i	X241r		X247i
# QPU	0   	0   	0   		3
# 16	X8r 	X8i 	X9r 		X15i
# 17	X136r	X136i	X137r		X143i
# ...
# 31	X248r	X248i	X249r		X255i
# QPU	4   	4   	4   		7

# pass 2
    v8subs   r1,        ra_vpm_2, 8;      v8subs vr_setup, ra_vpm_2, 8
    brr      ra_link,   r5rep, -, r:pass_2_tw
    add.setf ra_addr,   unif, unif;       mov    vw_setup, r1     # <= destination address/2, msb = last flag
    add.ifc  ra_link,   r5, rb_end;       ldtmu0                  # return to :end on the last turn
    mov      r0,        r4;               mov r5rep, r4           # r5 = rb_tr_4
:end_base

                                          mov    vr_setup, ra_vpm_2
    add      rb_addr,   unif, rb_aoff_x;  ldtmu0                  # <= next source address, <- twiddles
    fsub     vpm,       r0, r1;           mov    t0s,      rb_addr# -> X0, X128, next batch
    fadd     vpm,       r0, r1;           fmul   r5rep,    r4, ra_m1f_x400.16b
    mov      r0,        r4;               mov    vw_setup, ra_vpm_2 # r5 = rb_tr_4
    .back 3
    brr      ra_link,   r:pass_2_tw
    .endb

    brr      ra_link,   r:loop, ra_sync
    add      t1s,       rb_addr, ra_m1f_x400.16a; mov r2,  rb_addr
    fsub     vpm,       r0, r1;           v8adds  t0s,     r2, 8
    fadd     vpm,       r0, r1;           v8adds  rb_addr, r2, 8

# VPM	0   	1   	2   	...	15	QPU
# 0 	X0r 	X0i 	X2r 		X14i	0
# 1 	X128r	X128i	X130r		X142i	1
# 2 	X64r	X64i	X66r		X78i	2
# 3 	X192r	X192i	X194r		X206i	3
# ...
# 7 	X224r	X224i	X226r		X238i	7
# 8 	X16r	X16i	X18r		X30i	0
# ...
# 15	X240r	X240i	X242r		X254i	7
# 16	X1r 	X1i 	X3r 		X15i	0
# 17	X129r	X129i	X131r		X143i	1
# ...
# 31	X241r	X241i	X243r		X255i	7

# VPM	0   	1   	2   	...	15
# 0 	Y0r 	Y0i 	Y64r		Y112i
# 1 	Y1r 	Y1i 	Y65r		Y113i
# 2 	Y2r 	Y2i 	Y66r		Y114i
# 3 	Y3r 	Y3i 	Y67r		Y115i
# ...
# 7 	Y7r 	Y7i 	Y71i		Y119i
# 8 	Y8r 	Y8i 	Y72i		Y120i
# ...
# 15	Y15r	Y15i	Y79r		Y127i
# 16	Y128r	Y128i	Y192r		Y240i
# 17	Y129r	Y129i	Y193r		Y241i
# ...
# 31	Y143r	Y143i	Y207r		Y255i

    add      t1s,    rb_addr, r2;         mov    vw_setup, ra_vdw     # -> X1, X129, next batch
    add      ra_addr, ra_addr, r3;        mov    vw_addr,  ra_addr
                                          mov    vw_setup, rb_vpm_1;  ldtmu0
    .back 3
    brr      ra_link, r:pass_1
    .endb

    mov.ifcc r3,     r1;                  add    vw_setup, ra_vdw, 16 # .if:3
    mov      t0s,    rb_tw_20;            mov    vw_addr,  ra_addr    # -> Twiddles for pass 2
    mov      vpm,    r3;                  mov.ifcc r0,     r1<<8
# r3 = [0:x0r, x8r, x4r, x12r, x2r, x10r, x6r, x14r, 8:x1r ... x15r]
# r0 = [0:x0i, x8i, x4i, x12i, x2i, x10i, x6i, x14i, 8:x1i ... x15i]
    mov      vpm,    r0;                  ldtmu0 # <- X0
    .back 3
    brr      ra_link, r:pass_1
    .endb

    mov      ra_link, rb_loop
    mov.ifcc r3,     r1;                  mov.ifcc r0,     r1<<8      # .if:3
    mov      vpm,    r3;                  read vw_wait
    mov      vpm,    r0;                  mov    t0s,      rb_tw_21
    .back 3
    brr      -,      r:loop, ra_sync
    .endb

:end
                                          mov    vr_setup, ra_vpm_2
    fsub     vpm,    r0, r1;              ldtmu0                      # <- X1
    fadd     vpm,    r0, r1;              fmul   r5rep,    r4, ra_m1f_x400.16b # r5 = rb_tr_4
    mov      r0,     r4;                  mov    vw_setup, ra_vpm_2
    .back 3
    brr      ra_link, r:pass_2_tw
    .endb

    brr      ra_link, r:loop, ra_sync
    fsub     vpm,    r0, r1
    fadd     vpm,    r0, r1
    mov.setf -,      ra_inst.8c

    mov      vw_setup, ra_vdw
    mov      vw_addr,  ra_addr
    add      vw_setup, ra_vdw, 16;        mov    r0,       -1
    add      vw_addr,  ra_addr, r3
    .back 3
    brr.allz -,        r:end_master
    .endb
#end_slave
    read     vw_wait;                     thrend
    srel     -, 0
    nop

:end_master
    .rep i, 7
    sacq     -, 0
    .endr
    mov      interrupt, r0;  read vw_wait;  thrend
    nop
    nop
    

##############################################################################
# SUBROUTINES

#.align 64, :loop
# needs ldtmu0 before call!!!
# input:
#   r4 =   [0:x0r, x4r,  x2r,  x6r,  x1r, x5r,  x3r,  x7r,  8:x0i ... x7i]
#   *t1s = [0:x8r, x12r, x10r, x14r, x9r, x13r, x11r, x15r, 8:x8i ... x15i]
# output:
#   r1 = [0:x0r, x8r, x4r, x12r, x2r, x10r, x6r, x14r, 8:x0i ... x14i]
#   r0 = [0:x1r, x9r, x5r, x13r, x3r, x11r, x7r, x15r, 8:x1i ... x15i]
#   r3 = [                                             8:x1r ... x15r]
#   C =  [0:0 ...                                0,    8:1   ... 1] = e:3
:pass_1
    shl.setf -, elem_num, rb_m1;   mov r0, r4;  ldtmu1          # C = e:1, N = e:0
.if 0
    #mov vpm, r0
    #mov vpm, r4
    #itof vpm, elem_num
    #itof vpm, ra_inst.8a
    mov vpm, rb_tr_2
    mov vpm, ra_ti_2
    nop
.else
# r0 = [0:X0r,   X64r,  X32r,  X96r,  X16r,  X80r,  X48r,  X112r, 8:X0i   ... X112i]
# r4 = [0:X128r, X192r, X160r, X224r, X144r, X208r, X176r, X240r, 8:X128i ... X240i]
    fsub r2, r4, r0
    fadd r0, r4, r0;              fmul.ifn r1, r2>>1, (ra_m1f_x400>>1).16b # .if:0
.if 0
    mov vpm, r0
    mov vpm, r2
.else
# r0 =  [0:x0r, x4r,  x2r,  x6r,  x1r, x5r,  x3r,  x7r,  8:x0i ... x7i]
# r2 = -[0:x8r, x12r, x10r, x14r, x9r, x13r, x11r, x15r, 8:x8i ... x15i]
# fr0  = xr0 + xr4*1  - xi4*0  = r0' = r0     + r0<<4           [0..3]
# fr8  = xr8 + xr12*0 - xi12*D = r0' = -r2>>4 + (r2*rb_dir)<<8  [4..7]
# fi0  = xi0 + xi4*1  + xr4*0  = r0' = r0     + r0<<4           [8..11]
# fi8  = xi8 + xi12*0 + xr12*D = r0' = -r2>>4 + (r2*rb_dir)>>8  [12..15]
# fr4  = xr0 - xr4*1  + xi4*0  = r2' = r0     - r0<<4           [0..3]
# fr12 = xr8 - xr12*0 + xi12*D = r2' = -r2>>4 - (r2*rb_dir)<<8  [4..7]
# fi4  = xi0 - xr4*1  - xi4*0  = r2' = r0     - r0<<4           [8..11]
# fi12 = xi8 - xr12*0 - xi12*D = r2' = -r2>>4 - (r2*rb_dir)>>8  [12..15]
    mov.ifnn r1, r0;              fmul.ifn r2, r2<<8, ra_dir_1  # .if:0
                                  mov.ifnn r2, r0<<1
    fadd     r0, r1, r2;
# r0    = [0:x0r, x8r,  x2r, x10r, x1r, x9r,  x3r, x11r, 8:x0i ... x11i]
# r1-r2 = [0:x4r, x12r, x6r, x14r, x5r, x13r, x7r, x15r, 8:x4i ... x15i]
.if 0
    mov vpm, r0
    fsub vpm, r1, r2
.else
    fsub.ifc r3, r1, r2;          mov.ifcc r3, r0<<2            # .if:1
                                  fmul  ra_tmp, r3, rb_tr_2
# r0 = [0:x0r, x1r, x4r, x5r, 4:x8r  ... x13r, 8:x0i ... x13i]
# r1 = [0:x2r, x3r, x6r, x7r, 4:x10r ... x15r, 8:x2i ... x15i]
# fr0 = xr0 + xr2*tr - xi2*ti = r0' = r0 + r1*tr - (r1* ti)<<8  [0..7]
# fi0 = xi0 + xi2*tr + xr2*ti = r0' = r0 + r1*tr - (r1*-ti)>>8  [8..15]
# fr2 = xr0 - xr2*tr + xi2*ti = r2' = r0 - r1*tr + (r1* ti)<<8  [0..7]
# fi2 = xi0 - xr2*tr - xi2*ti = r2' = r0 - r1*tr + (r1*-ti)>>8  [8..15]
    fsub.ifcc r2, r1, r2;         fmul     r1, r3<<8, ra_ti_2
    fsub     r1, ra_tmp, r1;      mov.ifc  r0, r2>>2            # .if:1
    fadd     r2, r0, r1;          
# r2    = [0:x0r, x8r,  x4r, x12r, x1r, x9r,  x5r, x13r, 8:x0i ... x13i]
# r0-r1 = [0:x2r, x10r, x6r, x14r, x3r, x11r, x7r, x15r, 8:x2i ... x15i]
.if 0
    mov vpm, r2
    fsub vpm, r0, r1
.else
    add.setf -, ra_elem, ra_elem;                               # C = e:3, N = e:2
    fsub.ifn r3, r0, r1;          mov.ifnn r3, r2<<4            # .if:2
                                  fmul ra_tmp, r3,    rb_tr_3   
    fsub.ifnn r0, r0, r1;         fmul     r1, r3<<8, ra_ti_3
    fsub     r1, ra_tmp, r1;      mov.ifn  r2, r0>>4            # .if:2
    fsub     r0, r2, r1
    fadd     r1, r2, r1;          mov.ifc  r3, r0>>8
.endif
.endif
.endif
.endif
    .back 3
    bra -, ra_link
    .endb

#.align 64, :loop
# unpack twiddle factors for pass 2
# input:
#   r0 = r4 = [t01, t30, t20, t31, t10, t32, t21, t33, t00, t34, t22, t35, t11, t36, t23, t37]
#   r5 = rb_tr_4 = loop 1: +t01, loop 2: -t01
# output:
#   r0-r1 = [x2r, x2i, x3r ... 8:x10r ... x15i]
#   r0+r1 = [x0r, x0i, x1r ... 8:x8r  ... x13i]
:pass_2_tw
.if 0
    bra -, ra_link_1
    nop;#mov vpm, ra_ti_2
    nop;#mov vpm, rb_tr_2
    nop
.endif
    mov      r1, vpm;              fmul      ra_tmp, vpm, r5
    shl.setf -, ra_elem, 24;       mov       r5rep, r0<<8           # C = e:0, N = e:3
                                   fmul      r3, r5, rb_dir_2       # r3 = ra_ti_4
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
    fsub.ifc  r3, r1, r2;          fmul      ra_tmp, r5, rb_dir_2   # .if:3
    shl.setf  -, ra_elem, 24;      mov.ifcc  r3, r0<<8              # C = e:0, N = e:3, .if:3
    fsub.ifnn r1, r1, r2;          fmul.ifc  r2, r3>>1, ra_tmp>>1   # .if:30
    mov.ifn   r1, r5;              fmul.ifcc r2, r3<<1, ra_tmp<<1
    fsub.ifnn r1, ra_m1f_x400.8a, r5; mov.ifn r0, r1>>8             # .if:3
    shl.setf  -, ra_elem.16b, 24;  mov       r1, r1<<8              # C = e:0, N = e:2
                                   fmul      r3, r3, r1
    fsub      r2, r3, r2;          mov       r5quad, r4<<2
    mov       r3, r5;              fmul      ra_tmp, r5, rb_dir_2
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
    fsub.ifn  r5quad, ra_m1f_x400.8a, r5; mov.ifnn r3, r1<<4        # .if:2
    fsub.ifnn r0, r0, r2;          fmul.ifc  r2, r3>>1, ra_tmp>>1   # .if:0
                                   fmul.ifcc r2, r3<<1, ra_tmp<<1
                                   fmul      r3, r3, r5
    fsub      r2, r3, r2;          mov.ifn   r1, r0>>4              # .if:2
    mov.ifc   r3, r4;              mov.ifcc  r3, r4<<1              # .if:0
    fadd      r0, r1, r2;          fmul      ra_tmp, r3, rb_dir_2
.if 0
    fsub vpm, r1, r2
    mov vpm, r0
.else
# r0    = [x0r, x0i, x1r ... 8:x8r  ... x13i]
# r1-r2 = [x2r, x2i, x3r ... 8:x10r ... x15i]
    shl.setf  -, ra_elem.8c, 30;   mov       rb_tmp, r3<<2           # C = e:1, N = e:0
                                   fmul.ifc  rb_tmp, r3>>2, (ra_m1f_x400>>2).16b# .if:1
    fsub.ifc  r3, r1, r2;          mov.ifcc  r3, r0<<2              # .if:1
    fsub.ifcc r1, r1, r2;          fmul.ifn  r2, r3>>1, ra_tmp>>1   # .if:0
                                   fmul.ifnn r2, r3<<1, ra_tmp<<1
                                   fmul      r3, r3, rb_tmp
    fsub      r1, r3, r2;          mov.ifc   r0, r1>>2              # .if:1
.endif
.endif
.endif

    .back 3
    bra -, ra_link
    .endb

#.align 64, :loop
# output:
#  r2 = 0x400
#  r3 = 0x200
:sync
    .rep i, 7
:       bra  -, ra_link
        srel -, 0
        shr  r3, ra_m1f_x400.16a, 1;  mov r2, ra_m1f_x400.16a
        sacq -, i+1
    .endr
:   # entry point for master
    shr  r3, ra_m1f_x400.16a, 1;  mov r2, ra_m1f_x400.16a
    .rep i, 7
        sacq -, 0
    .endr
    .rep i, 7
        srel -, i+1
    .endr
    .back 3
        bra -, ra_link
    .endb

