.set RES_COUNT, 38

shl r0, elem_num, 3;
mov ra0, unif # vector count
add ra1, r0, unif; # base address
mov ra3, unif; # target address
mov rb0, 16*2*4-4

:next
# setup VPM write
mov r2, vpm_setup(16, 1, v32(0,0))
add vw_setup, r2, elem_num;

add ra1, ra1, 4; mov t0s, ra1
ldtmu0
add ra1, ra1, rb0; mov t0s, ra1
mov r0, r4; ldtmu0
mov r1, r4

#xor vpm, r0, 0x40000000
#xor vpm, r1, 0x40000000
fadd vpm, r0, r1 # Y0
fsub vpm, r0, r1
fmin vpm, r0, r1
fmax vpm, r0, r1
fminabs vpm, r0, r1 # Y4
fmaxabs vpm, r0, r1
ftoi vpm, r1
itof vpm, r1
add vpm, r0, r1 # Y8
sub vpm, r0, r1
shr vpm, r0, r1
asr vpm, r0, r1
ror vpm, r0, r1 # Y12
shl vpm, r0, r1
min vpm, r0, r1
max vpm, r0, r1
and vpm, r0, r1 # Y16
or vpm, r0, r1
xor vpm, r0, r1
not vpm, r1
clz vpm, r1 # Y20
fmul vpm, r0, r1
mul24 vpm, r0, r1
v8muld vpm, r0, r1
v8min vpm, r0, r1 # Y24
v8max vpm, r0, r1
v8adds vpm, r0, r1
v8subs vpm, r0, r1
.long 0x009e7040, 0x10020c27 # Anop, Y28
.long 0x099e7040, 0x10020c27 # AddOp 9
.long 0x0a9e7040, 0x10020c27 # AddOp 10
.long 0x0b9e7040, 0x10020c27 # AddOp 11
.long 0x199e7040, 0x10020c27 # AddOp 25, Y32
.long 0x1a9e7040, 0x10020c27 # AddOp 26
.long 0x1b9e7040, 0x10020c27 # AddOp 27
.long 0x1c9e7040, 0x10020c27 # AddOp 28
.long 0x1d9e7040, 0x10020c27 # AddOp 29, Y36
.long 0x009e7001, 0x100049f0 # Mnop

# dma write
mov r2, 16*4; mov r1, ra3
.rep i, (RES_COUNT-1)/16
mov vw_setup, vdw_setup_1((RES_COUNT-16)*4)
mov vw_setup, vdw_setup_0(16, 16, dma_h32(16*i,0))
add r1, r1, r2; mov vw_addr, r1
mov -, vw_wait
.endr
mov vw_setup, vdw_setup_1(((RES_COUNT-1)&-16)*4)
mov vw_setup, vdw_setup_0(16, ((RES_COUNT-1)&15)+1, dma_h32((RES_COUNT-1)&-16,0))
mov vw_addr, r1

sub.setf ra0, ra0, 1
brr.anynz -, :next
mov r2, RES_COUNT*16*4
add ra3, ra3, r2
mov -, vw_wait

thrend
mov interrupt, 1;
nop
