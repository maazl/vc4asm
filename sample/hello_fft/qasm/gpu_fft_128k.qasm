# BCM2835 "GPU_FFT" release 3.0
#
# Copyright (c) 2015, Andrew Holme.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the copyright holder nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.set STAGES, 17

.include "gpu_fft_ex.qinc"

##############################################################################
# Twiddles: src

.set TW64_BASE0,    0 # rx_tw_shared
.set TW64_BASE1,    1
.set TW32_BASE,     2
.set TW16_BASE,     3
.set TW48_P2_STEP,  4
.set TW64_P2_STEP,  5
.set TW32_P2_STEP,  6
.set TW16_P2_STEP,  7
.set TW32_P3_STEP,  8
.set TW16_P3_STEP,  9

.set TW32_P3_BASE,  0 # rx_tw_unique
.set TW16_P3_BASE,  1

##############################################################################
# Twiddles: dst

.set TW16_STEP, 0  # 1
.set TW32_STEP, 1  # 1
.set TW16,      2  # 5
.set TW32,      7  # 2
.set TW48,      9  # 2
.set TW64,      11 # 2
.set TW48_STEP, 13 # 1
.set TW64_STEP, 14 # 1

##############################################################################
# Registers

.set ra_link_0,         ra0
.set rb_pass2_link,     rb0
.set ra_save_ptr,       ra1
.set rx_inst,           rb1
.set ra_temp,           ra2
.set rx_vpm,            rb2
.set ra_addr_x,         ra3
#                       rb3
.set ra_save_64,        ra4
.set rx_0x55555555,     rb4
.set ra_load_idx,       ra5
.set rx_0x33333333,     rb5
.set ra_sync,           ra6
.set rx_0x0F0F0F0F,     rb6
.set ra_points,         ra7
.set rb_0x1D0,          rb7
.set ra_link_1,         ra8
.set rb_link_1,         rb8

.set ra_32_re,          ra9
.set rb_32_im,          rb9
.set ra_vdw_32,         ra10
.set rb_0xF0,           rb10

.set ra_64,             ra11 # 4
.set rb_64,             rb11 # 4

.set rx_tw_shared,      ra15
.set rx_tw_unique,      rb15

.set ra_tw_re,          ra16 # 15
.set rb_tw_im,          rb16 # 15

##############################################################################
# Constants

mov rb_0xF0,       0xF0
mov rb_0x1D0,      0x1D0
mov rx_0x55555555, 0x55555555
mov rx_0x33333333, 0x33333333
mov rx_0x0F0F0F0F, 0x0F0F0F0F

mov ra_vdw_32, vdw_setup_0(32, 16, dma_h32( 0,0))

##############################################################################
# Twiddles: ptr

init_tw

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
    mov r3, unif;              mov ra_save_64, 0
    shl.setf r0, r3, 5;        mov ra_sync, 0
    mov.ifnz r1, :sync_slave - :sync - 4*8 # -> rx_inst-1
    add.ifnz ra_sync, r1, r0;  mov rx_inst, r3
    mov.ifnz r1, :save_slave_64 - :save_64 - 4*8 # -> rx_inst-1
    add.ifnz ra_save_64, r1, r0;
    
# (MM) Optimized: reduced VPM registers to 1
inst_vpm r3, rx_vpm

##############################################################################
# Top level

    ;mov ra_addr_x, unif  # Ping buffer or null
    # (MM) Optimized: check loop condition below, load target buffer in init_stage
:loop

##############################################################################
# Pass 1

    # (MM) More powerful init macros to simplify code
    init_base_64 TW16_BASE, TW32_BASE, TW64_BASE0, TW64_BASE1
    read_rev 0x10

    # (MM) Optimized: place branch before the last two instructions of read_rev
    .back 2
    brr ra_link_1, r:pass_1
    .endb
    mov ra_points, (1<<STAGES) / 0x200 - 1

:   # start of hidden loop
    # (MM) Optimized: branch unconditional and patch the return address
    # for the last turn.
    brr r0, r:pass_1
    sub.setf ra_points, ra_points, 1
    mov.ifz ra_link_1, r0
    nop

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 2

    # (MM) More powerful init macros to simplify code
    init_step_64 TW16_BASE, TW32_BASE, TW64_BASE0, TW64_BASE1, TW16_P2_STEP, TW32_P2_STEP, TW48_P2_STEP, TW64_P2_STEP
    read_lin 0x10

    # (MM) Optimized: keep return address additionally in rb_link_1 for loop.
    # and setup for loop below
    .back 1
    brr ra_link_1, rb_link_1, -, r:pass_2
    .endb
    mov ra_points, (1<<STAGES) / 0x200 - 1
    mov rb_pass2_link, :3f - :2f

:   # start of hidden loop
    .rep i, 2
    brr ra_link_1, r:pass_2
    nop
    nop
    nop
    .endr

    # (MM) Optimized: patch the return address for the last turn to save the
    # conditional branch and the unecessary twiddle load after the last turn.
    brr ra_link_1, r0, -, r:pass_2
    sub.setf ra_points, ra_points, 4
    add.ifn ra_link_1, r0, rb_pass2_link
    nop
:2
    next_twiddles_64

    mov ra_link_1, rb_link_1
    brr_opt -, r:pass_2, 3
:3
    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 3

    # (MM) More powerful init macros to simplify code
    init_last_32 TW16_P3_BASE, TW32_P3_BASE, TW16_P3_STEP, TW32_P3_STEP
    read_lin 0x10

    # (MM) Optimized: place branch before the last two instructions of read_lin
    .back 2
    brr ra_link_1, r:pass_3
    .endb
    mov ra_points, (1<<STAGES) / 0x100 - 1

:   # start of hidden loop
    next_twiddles_32

    # (MM) Optimized: branch unconditional and patch the return address of the last turn.
    sub.setf ra_points, ra_points, 1
    mov.ifz ra_link_1, r0
    brr_opt r0, r:pass_3, 3

    # (MM) Optimized: redirect ra_link_1 to :loop to save branch and 3 nop.
    # Also check loop condition immediately.
    mov.setf ra_addr_x, unif;  ldtmu0  # Ping buffer or null
    # (MM) Optimized: easier procedure chains
    brr r0, ra_link_1, r:sync, ra_sync
    mov r1, r:loop - r:1f
    add.ifnz ra_link_1, r0, r1
    ldtmu0
:1

##############################################################################

    # (MM) flag obsolete
    exit

# (MM) Optimized: easier procedure chains
##############################################################################
# Subroutines

# (MM) Optimized: joined load_xxx and ldtmu in FFT-16 codelet
bodies_fft_16
    .back 3
    bra -, ra_link_0
    .endb

:pass_1
    body_pass_64 LOAD_REVERSED, rb_0x1D0

    # (MM) Optimized procedure chains
    .back 3
    brr -, ra_save_64, r:save_64
    .endb

:save_64
    body_ra_save_64

:save_slave_64
    body_rx_save_slave_64

:pass_2
    body_pass_64 LOAD_STRAIGHT, rb_0x1D0

    # (MM) Optimized procedure chains
    .back 3
    brr -, ra_save_64, r:save_64
    .endb

:pass_3
    body_pass_32 LOAD_STRAIGHT

    # (MM) link to slave procedure without need for a register
    .back 3
    ;mov.setf -, rx_inst
    brr.allnz -, r:save_slave
    .endb

#:save_32
    body_ra_save_32

:save_slave
    body_rx_save_slave

:sync
    body_ra_sync

:sync_slave
    body_rx_sync_slave

