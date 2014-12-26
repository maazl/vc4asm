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

# dma write
mov vw_setup, vdw_setup_1((28-16)*4)
mov vw_setup, vdw_setup_0(16, 16, dma_h32(0,0))
mov vw_addr, ra3
mov -, vw_wait
mov vw_setup, vdw_setup_1((28-12)*4)
mov vw_setup, vdw_setup_0(16, 12, dma_h32(16,0))
mov r2, 16*4
add r1, ra3, r2
mov vw_addr, r1

sub.setf ra0, ra0, 1
brr.anynz -, :next
mov r2, 28*16*4
add ra3, ra3, r2
mov -, vw_wait

thrend
mov interrupt, 1;
nop
